#include "system/JoystickSetup.h"

#include "core/Config.h"
#include "core/Display.h"
#include "core/Storage.h"
#include "core/Text.h"

namespace
{
    const char* const NAMES[] = {"SENSITIVITY", "DEBOUNCE"};
    constexpr uint8_t ITEM_CALIBRATE = 2;
    constexpr uint8_t ITEM_TEST = 3; // пустой пункт: стик можно двигать, ничего не меняя
    constexpr uint8_t ITEM_COUNT = 4;

    constexpr uint16_t CENTER_MS = 2000; // отпустить стик; центр - среднее за вторую секунду
    constexpr uint16_t ROTATE_MS = 6000; // крутить по кругу у края
    constexpr uint16_t RESULT_MS = 1500;

    // "ROTATE STICK  5": строка и оставшиеся секунды
    void countdown(uint8_t row, const char* text, uint32_t leftMs)
    {
        char buf[Display::COLS + 1];
        char* p = Text::str(buf, text);
        while (p < buf + Display::COLS - 1) p = Text::chr(p, ' ');
        Text::num(p, (leftMs + 999) / 1000);
        Display::printRow(row, buf);
    }
} // namespace

void JoystickSetup::begin()
{
    sensitivity = Storage::loadSetting(Storage::JOY_SENSITIVITY, config::DEFAULT_SENSITIVITY);
    debounce = Storage::loadSetting(Storage::JOY_DEBOUNCE, config::DEFAULT_DEBOUNCE);
    Display::clear();
    drawSetting();
}

// "SENSITIVITY < 5>" / "DEBOUNCE   <30ms>" / "CALIBRATE: OK  →" / "TEST: MOVE STICK"
void JoystickSetup::drawSetting()
{
    if (item == ITEM_TEST)
    {
        Display::printCentered(1, "TEST: MOVE STICK");
        return;
    }
    if (item == ITEM_CALIBRATE)
    {
        // \x7E - стрелка вправо в знакогенераторе HD44780: "→ - начать"
        Display::printRow(1, Input::calibrated() ? "CALIBRATE: OK  \x7E" : "CALIBRATE: --  \x7E");
        return;
    }
    char row[Display::COLS + 1];
    char* p = Text::str(row, NAMES[item]);
    uint8_t value = item == 0 ? sensitivity : debounce;
    uint8_t width = item == 0 ? 2 : 3; // "< 5>" или "< 30ms>", до "<100ms>"
    uint8_t tail = item == 0 ? 4 : 7;
    while (p < row + Display::COLS - tail) p = Text::chr(p, ' ');
    p = Text::num(Text::chr(p, '<'), item == 0 ? value : value * 10, width, ' ');
    if (item == 1) p = Text::str(p, "ms");
    Text::chr(p, '>');
    Display::printRow(1, row);
}

// "X 512Y 498 LRUDC": точка - направление не нажато.
void JoystickSetup::drawLive()
{
    char row[Display::COLS + 1];
    char* p = Text::num(Text::chr(row, 'X'), Input::rawX(), 4, ' ');
    p = Text::chr(Text::num(Text::chr(p, 'Y'), Input::rawY(), 4, ' '), ' ');
    const char flags[] = "LRUDC";
    for (uint8_t i = 0; i < 5; i++) p = Text::chr(p, Input::held(1 << i) ? flags[i] : '.');
    Display::printRow(0, row);
}

void JoystickSetup::startCalibration()
{
    calib = Calib::Center;
    sumX = sumY = 0;
    samples = 0;
    Display::printCentered(0, "RELEASE STICK");
    phase.reset();
}

// Центр: стик отпущен, среднее за вторую секунду (первая - чтобы стик успел вернуться).
// Края: пока стик крутят, запоминаются наименьшие и наибольшие значения осей.
void JoystickSetup::updateCalibration()
{
    uint16_t x = Input::rawX(), y = Input::rawY();
    switch (calib)
    {
    case Calib::Center:
        if (phase.elapsed() >= CENTER_MS / 2)
        {
            sumX += x;
            sumY += y;
            samples++;
        }
        if (phase.elapsed() < CENTER_MS)
        {
            countdown(1, "CENTER", CENTER_MS - phase.elapsed());
            return;
        }
        cal.midX = sumX / samples;
        cal.midY = sumY / samples;
        cal.minX = cal.maxX = cal.midX;
        cal.minY = cal.maxY = cal.midY;
        calib = Calib::Rotate;
        Display::printCentered(0, "ROTATE STICK");
        phase.reset();
        return;
    case Calib::Rotate:
        if (x < cal.minX) cal.minX = x;
        if (x > cal.maxX) cal.maxX = x;
        if (y < cal.minY) cal.minY = y;
        if (y > cal.maxY) cal.maxY = y;
        if (phase.elapsed() < ROTATE_MS)
        {
            countdown(1, "AT EDGE, ROUNDS", ROTATE_MS - phase.elapsed());
            return;
        }
        if (Input::validCalibration(cal))
        {
            Input::setCalibration(cal);
            Display::printCentered(0, "CALIBRATED");
            Display::printCentered(1, "SAVED");
        }
        else
        {
            Display::printCentered(0, "CALIBRATION FAIL");
            Display::printCentered(1, "TRY AGAIN");
        }
        calib = Calib::Result;
        phase.reset();
        return;
    default: // Result
        if (phase.elapsed() < RESULT_MS) return;
        calib = Calib::Off;
        Input::suppressUntilRelease(); // стик ещё могут держать наклонённым
        drawSetting();
        return;
    }
}

void JoystickSetup::update()
{
    if (calib != Calib::Off)
    {
        updateCalibration();
        return;
    }
    if (Input::clickHeldMs() >= config::HOLD_TO_EXIT_MS)
    {
        Storage::saveSetting(Storage::JOY_SENSITIVITY, sensitivity);
        Storage::saveSetting(Storage::JOY_DEBOUNCE, debounce);
        requestExit();
        return;
    }
    if (Input::released(Input::CLICK) && Input::lastClickMs() < config::HOLD_TO_EXIT_MS)
    {
        item = (item + 1) % ITEM_COUNT;
        drawSetting();
    }
    int8_t d = Input::repeat(Input::RIGHT) ? 1 : Input::repeat(Input::LEFT) ? -1 : 0;
    if (d && item == ITEM_CALIBRATE)
    {
        if (d > 0) startCalibration();
        return;
    }
    if (d && item != ITEM_TEST)
    {
        uint8_t& v = item == 0 ? sensitivity : debounce;
        v = constrain(v + d, 1, 10);
        if (item == 0)
            Input::setSensitivity(v);
        else
            Input::setDebounce(v);
        drawSetting();
    }
    if ((uint16_t)millis() - lastDraw >= 100)
    {
        lastDraw = millis();
        drawLive();
    }
}
