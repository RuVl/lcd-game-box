// Шаги внутри состояния конечного автомата без delay(): шаг переключается по таймеру
// или по событию, loop() при этом не ждёт.
#pragma once

#include <Arduino.h>

struct Phase {
  uint8_t step = 0;
  uint32_t at = 0;  // когда начался текущий шаг

  void reset() {
    step = 0;
    at = millis();
  }
  void next() {
    step++;
    at = millis();
  }
  void restartTimer() { at = millis(); }
  uint32_t elapsed() const { return millis() - at; }
};
