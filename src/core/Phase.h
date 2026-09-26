// Шаги внутри состояния конечного автомата без delay(): шаг переключается по таймеру
// или по событию, loop() при этом не ждёт.
//
// Время 16-битное (младшие биты millis()): на 8-битном AVR это заметно короче 32-битного кода.
// elapsed() верен до 65 с - все паузы в играх намного короче.
#pragma once

#include <Arduino.h>

struct Phase
{
    uint8_t step = 0;
    uint16_t at = 0; // когда начался текущий шаг

    void reset()
    {
        step = 0;
        at = millis();
    }

    void next()
    {
        step++;
        at = millis();
    }

    void restartTimer() { at = millis(); }
    uint16_t elapsed() const { return (uint16_t)millis() - at; }
};
