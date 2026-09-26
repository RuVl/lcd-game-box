#include "core/Sound.h"

#include "core/Config.h"

namespace Sound
{
    namespace
    {
        const Note* current = nullptr;
        uint32_t nextAt = 0;
    } // namespace

    void begin() { pinMode(config::PIN_BUZZER, OUTPUT); }

    void play(const Note* progmemMelody)
    {
        current = progmemMelody;
        nextAt = 0;
    }

    void stop()
    {
        current = nullptr;
        noTone(config::PIN_BUZZER);
    }

    bool playing() { return current != nullptr; }

    void update()
    {
        if (!current || (int32_t)(millis() - nextAt) < 0) return;
        Note n;
        memcpy_P(&n, current, sizeof(Note));
        if (n.ms == 0)
        {
            stop();
            return;
        }
        // Ноту запускаем без длительности и гасим сами. С длительностью tone() выключает
        // звук в прерывании, и если это совпадает с запуском следующей ноты (а она запускается
        // ровно тогда), прерывание может вызвать noTone() для "пина 255". Тогда D9 остаётся
        // в HIGH: постоянный ток через катушку зуммера даёт шипение, пока не заиграет новый звук.
        if (n.freq > 1)
            tone(config::PIN_BUZZER, n.freq);
        else
            noTone(config::PIN_BUZZER);
        nextAt = millis() + n.ms;
        current++;
    }
} // namespace Sound
