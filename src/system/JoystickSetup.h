// Настройка джойстика: калибровка, чувствительность и защита от дребезга, плюс живые показания осей.
//
//   X 512Y 498 L.U.C     ← сырые оси и нажатые направления (L R U D C)
//   SENSITIVITY < 5>     ← пункт: нажатие - следующий, ←/→ - значение 1…10
//
// Пункты: SENSITIVITY, DEBOUNCE, CALIBRATE (→ - начать), TEST (←/→ ничего не меняют, стик
// можно просто подвигать и посмотреть оси). Значения применяются сразу, в EEPROM сохраняются
// при выходе (удержание стика); калибровка сохраняется сразу после успешного прохода.
//
// Калибровка: 2 с отпустить стик (запоминается центр), затем 6 с крутить его по кругу у самого
// края (запоминаются крайние значения по каждой оси).
#pragma once

#include <Arduino.h>

#include "core/Game.h"
#include "core/Input.h"
#include "core/Phase.h"

class JoystickSetup : public Game
{
public:
    void begin() override;
    void update() override;

private:
    enum class Calib : uint8_t { Off, Center, Rotate, Result };

    uint8_t sensitivity, debounce; // уровни 1…10
    uint8_t item; // 0 - чувствительность, 1 - дребезг, 2 - калибровка, 3 - просто проверка
    uint16_t lastDraw;

    Calib calib;
    Phase phase;
    uint32_t sumX, sumY;
    uint16_t samples;
    Input::Calibration cal;

    void drawSetting();
    void drawLive();
    void startCalibration();
    void updateCalibration();
};
