// Flappy Bird: константы и звуки. Подключается только из FlappyGame.cpp.
#pragma once

#include "core/Sound.h"

namespace flappy {

constexpr uint16_t TICK_MS = 40;     // один шаг физики
constexpr uint8_t HISCORE_SLOT = 5;  // слот рекорда в EEPROM

// Холст и птица (в точках). Позиции и скорости — в 1/16 точки.
constexpr uint8_t FIELD_W = 20;
constexpr uint8_t FIELD_H = 16;
constexpr uint8_t BIRD_X = 4;
constexpr uint8_t BIRD_SIZE = 2;
constexpr uint8_t TEXT_COL = 5;  // отсюда до конца строки — текст

constexpr int8_t GRAVITY = 3;    // прибавка скорости за шаг
constexpr int8_t FLAP_V = -18;   // скорость после взмаха: подъём ≈4 точки за ≈0,25 с
constexpr int8_t MAX_FALL = 20;  // предельная скорость падения

// Трубы
constexpr uint8_t PIPE_W = 2;
constexpr uint8_t PIPE_SPACING = 11;  // 2 трубы × 11 = 22 = ширина холста + ширина трубы
constexpr uint8_t PIPE_FIRST_X = 24;  // первая труба чуть за правым краем
// Как в оригинале: скорость постоянная, проём ≈ 4 высоты птицы (там 100 px при птице 24 px).
constexpr int16_t SPEED = 4;  // 0,25 точки за шаг ≈ 6 точек/с
constexpr uint8_t GAP = 8;

const Note SND_FLAP[] PROGMEM = {{1568, 25}, {2093, 25}, {0, 0}};
const Note SND_POINT[] PROGMEM = {{988, 60}, {1319, 120}, {0, 0}};
const Note SND_CRASH[] PROGMEM = {{880, 60}, {659, 60}, {523, 80}, {440, 200}, {0, 0}};
const Note SND_START[] PROGMEM = {{784, 70}, {1047, 70}, {1319, 120}, {0, 0}};

}  // namespace flappy
