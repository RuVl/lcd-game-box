// Шаг игры с фиксированным (или меняющимся) периодом, без delay().
//
//   Ticker ticker;                        // поле класса игры
//   ticker.start();                       // при входе в игру (и после паузы)
//   if (!ticker.due(TICK_MS)) return;     // каждый проход update(): true - пора делать шаг
//
// Время 16-битное (младшие биты millis()): на 8-битном AVR это заметно короче 32-битного кода.
// Годится для периодов до нескольких секунд. После паузы (игру не обновляли - экран итога,
// заставка) снова вызвать start(): отставание больше 32 с 16-битное время уже не различит.
#pragma once

#include <Arduino.h>

class Ticker
{
public:
    void start(uint16_t delayMs = 0) { next_ = (uint16_t)millis() + delayMs; }

    bool due(uint16_t periodMs);

private:
    uint16_t next_ = 0; // когда следующий шаг
};
