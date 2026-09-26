#include "core/Input.h"

#include <EncButton.h>

#include "core/Config.h"
#include "core/Storage.h"

namespace Input
{
    namespace
    {
        constexpr uint8_t COUNT = 5; // LEFT, RIGHT, UP, DOWN, CLICK - в порядке битов маски

        VirtButton buttons[COUNT];
        bool axisState[4]; // состояние направлений с гистерезисом
        int lastX, lastY, midX, midY;
        int minX, maxX, minY, maxY; // края осей из калибровки
        bool hasCalibration;
        int offX, offY; // наклон от центра: вправо и вверх - положительные
        uint8_t onPct; // порог наклона, % хода стика от центра до края (из чувствительности)
        uint16_t lastSample;
        uint8_t heldMask, prevHeldMask;
        bool suppressed = true; // кнопка, зажатая при включении, не считается нажатием
        // События (нажал/отпустил/автоповтор) не выдаются и на том проходе, где блокировка снялась:
        // иначе отпускание кнопки, зажатой с прошлого экрана, выглядело бы как клик.
        bool eventsBlocked = true;
        uint16_t clickStart, clickDuration;

        // ---- чтение осей ----
#if defined(ESP8266)
        // Одна ось на A0, вторая "отключена" выводом MUX в LOW.
        int readAxis(uint8_t enablePin, uint8_t disablePin)
        {
            pinMode(disablePin, OUTPUT);
            digitalWrite(disablePin, LOW);
            pinMode(enablePin, INPUT);
            delayMicroseconds(30); // перезаряд входа АЦП через 1 кОм и диод
            return analogRead(config::PIN_JOY_ADC);
        }
        int readX() { return readAxis(config::PIN_MUX_X, config::PIN_MUX_Y); }
        int readY() { return readAxis(config::PIN_MUX_Y, config::PIN_MUX_X); }
#else
        int readX() { return analogRead(config::PIN_JOY_X); }
        int readY() { return analogRead(config::PIN_JOY_Y); }
#endif

        // Направление с гистерезисом: включается дальше порога, выключается ближе 3/5 порога.
        // range - ход стика от центра до края в эту сторону.
        bool hysteresis(bool on, int offset, int range)
        {
            int threshold = (long)range * onPct / 100;
            return offset > (on ? threshold * 3 / 5 : threshold);
        }

        void sampleAxes()
        {
            lastX = readX();
            lastY = readY();
            // Ход от центра к меньшим и к большим значениям. Без калибровки - симметричный
            // по ближнему краю (у ESP8266 из-за диода дальний край недостижим).
            int lowX = hasCalibration ? midX - minX : min(midX, 1023 - midX);
            int highX = hasCalibration ? maxX - midX : lowX;
            int lowY = hasCalibration ? midY - minY : min(midY, 1023 - midY);
            int highY = hasCalibration ? maxY - midY : lowY;
            // "вправо" и "вверх" - положительные; "вверх" без инверсии даёт меньшие значения
            int dxv = offX = config::JOY_INVERT_X ? midX - lastX : lastX - midX;
            int dyv = offY = config::JOY_INVERT_Y ? lastY - midY : midY - lastY;
            int rightRange = config::JOY_INVERT_X ? lowX : highX, leftRange = config::JOY_INVERT_X ? highX : lowX;
            int upRange = config::JOY_INVERT_Y ? highY : lowY, downRange = config::JOY_INVERT_Y ? lowY : highY;
            axisState[0] = hysteresis(axisState[0], -dxv, leftRange); // LEFT
            axisState[1] = hysteresis(axisState[1], dxv, rightRange); // RIGHT
            axisState[2] = hysteresis(axisState[2], dyv, upRange); // UP
            axisState[3] = hysteresis(axisState[3], -dyv, downRange); // DOWN
        }

        bool anyEvent(uint8_t mask, bool (VirtButton::*event)())
        {
            if (eventsBlocked) return false;
            for (uint8_t i = 0; i < COUNT; i++)
                if (mask & (1 << i) && (buttons[i].*event)()) return true;
            return false;
        }
    } // namespace

    void begin()
    {
        pinMode(config::PIN_JOY_SW, INPUT_PULLUP);
        // Центр стика - среднее нескольких чтений при включении (стик в этот момент не трогают).
        long sx = 0, sy = 0;
        for (uint8_t i = 0; i < 8; i++)
        {
            sx += readX();
            sy += readY();
            delay(2);
        }
        midX = sx / 8;
        midY = sy / 8;
        // Сохранённая калибровка. Центр при включении обычно точнее (стик мог чуть "уплыть"),
        // но если стик в этот момент держали наклонённым - берём сохранённый.
        Calibration c;
        Storage::loadBlock(Storage::JOY_CALIBRATION, &c, sizeof(c));
        if (validCalibration(c))
        {
            int bootX = midX, bootY = midY;
            setCalibration(c, false);
            if (abs(bootX - (int)c.midX) <= min(midX - minX, maxX - midX) / 6) midX = bootX;
            if (abs(bootY - (int)c.midY) <= min(midY - minY, maxY - midY) / 6) midY = bootY;
        }
        setSensitivity(Storage::loadSetting(Storage::JOY_SENSITIVITY, config::DEFAULT_SENSITIVITY));
        setDebounce(Storage::loadSetting(Storage::JOY_DEBOUNCE, config::DEFAULT_DEBOUNCE));
        for (VirtButton& b : buttons)
        {
            b.setHoldTimeout(config::REPEAT_DELAY_MS);
            b.setStepTimeout(config::REPEAT_STEP_MS);
        }
    }

    void update()
    {
        if ((uint16_t)millis() - lastSample >= config::JOY_SAMPLE_MS)
        {
            lastSample = millis();
            sampleAxes();
        }
        bool click = digitalRead(config::PIN_JOY_SW) == LOW;

        prevHeldMask = heldMask;
        heldMask = 0;
        for (uint8_t i = 0; i < COUNT; i++)
        {
            bool raw = i < 4 ? axisState[i] : click;
            buttons[i].tick(raw);
            if (buttons[i].pressing()) heldMask |= 1 << i;
        }

        clickDuration = 0;
        eventsBlocked = suppressed;
        if (suppressed)
        {
            if (heldMask == 0) suppressed = false;
            return;
        }
        if (buttons[4].press()) clickStart = millis();
        if (buttons[4].release()) clickDuration = (uint16_t)millis() - clickStart;
    }

    bool held(uint8_t mask) { return !suppressed && (heldMask & mask); }
    bool pressed(uint8_t mask) { return anyEvent(mask, &VirtButton::press); }
    bool released(uint8_t mask) { return anyEvent(mask, &VirtButton::release); }
    bool anyPressed(uint8_t mask) { return !suppressed && (heldMask & mask) && !(prevHeldMask & mask); }
    bool repeat(uint8_t mask) { return pressed(mask) || anyEvent(mask, &VirtButton::step); }

    int8_t dx()
    {
        if (held(LEFT)) return -1;
        if (held(RIGHT)) return 1;
        return 0;
    }

    uint16_t clickHeldMs() { return held(CLICK) ? (uint16_t)millis() - clickStart : 0; }
    uint16_t lastClickMs() { return clickDuration; }

    uint8_t direction()
    {
        uint8_t h = heldMask & (LEFT | RIGHT | UP | DOWN);
        if (!h || suppressed) return 0;
        bool xAxis = abs(offX) >= abs(offY);
        uint8_t pick = h & (xAxis ? (LEFT | RIGHT) : (UP | DOWN));
        return pick ? pick : h & (xAxis ? (UP | DOWN) : (LEFT | RIGHT));
    }

    void setSensitivity(uint8_t level)
    {
        level = constrain(level, 1, 10);
        onPct = 95 - (level - 1) * 65 / 9; // 1 → 95 % (почти до упора), 9 → 38 %, 10 → 30 %
    }

    void setDebounce(uint8_t level)
    {
        level = constrain(level, 1, 10);
        for (VirtButton& b : buttons) b.setDebTimeout(level * 10);
    }

    void suppressUntilRelease()
    {
        suppressed = true;
        eventsBlocked = true;
    }

    bool calibrated() { return hasCalibration; }

    bool validCalibration(const Calibration& c)
    {
        return c.maxX <= 1023 && c.maxY <= 1023 && c.minX + 100 <= c.midX && c.midX + 100 <= c.maxX &&
            c.minY + 100 <= c.midY && c.midY + 100 <= c.maxY;
    }

    void setCalibration(const Calibration& c, bool save)
    {
        minX = c.minX;
        midX = c.midX;
        maxX = c.maxX;
        minY = c.minY;
        midY = c.midY;
        maxY = c.maxY;
        hasCalibration = true;
        if (save) Storage::saveBlock(Storage::JOY_CALIBRATION, &c, sizeof(c));
    }

    int rawX() { return lastX; }
    int rawY() { return lastY; }
    int centerX() { return midX; }
    int centerY() { return midY; }
} // namespace Input
