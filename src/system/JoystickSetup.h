// Настройка джойстика: чувствительность и защита от дребезга, плюс живые показания осей.
//
//   X 512Y 498 L.U.C     ← сырые оси и нажатые направления (L R U D C)
//   SENSITIVITY < 5>     ← настройка: нажатие — следующая, ←/→ — значение 1…10
//
// Значения применяются сразу, в EEPROM сохраняются при выходе (удержание стика).
#pragma once

#include <Arduino.h>

#include "core/Game.h"

class JoystickSetup : public Game {
 public:
  void begin() override;
  void update() override;

 private:
  uint8_t sensitivity, debounce;  // уровни 1…10
  uint8_t item;                   // 0 — чувствительность, 1 — дребезг
  uint32_t lastDraw;

  void drawSetting();
  void drawLive();
};
