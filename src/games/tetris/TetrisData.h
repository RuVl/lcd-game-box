// Tetris: фигуры, таблицы скорости и очков, значки и мелодии в PROGMEM.
// Подключается только из TetrisGame.cpp.
#pragma once

#include "core/Sound.h"

namespace tetris
{
    enum : uint8_t { P_I, P_O, P_T, P_S, P_Z, P_J, P_L, PIECE_COUNT };

    // Фигуры в квадрате 4×4: 16 бит построчно сверху вниз, в каждой тетраде старший бит - левая клетка.
    // Повороты по часовой стрелке, как в SRS.
    const uint16_t SHAPES[PIECE_COUNT][4] PROGMEM = {
        {0x0F00, 0x2222, 0x00F0, 0x4444}, // I
        {0x6600, 0x6600, 0x6600, 0x6600}, // O
        {0x4E00, 0x4640, 0x0E40, 0x4C40}, // T
        {0x6C00, 0x4620, 0x06C0, 0x8C40}, // S
        {0xC600, 0x2640, 0x0C60, 0x4C80}, // Z
        {0x8E00, 0x6440, 0x0E20, 0x44C0}, // J
        {0x2E00, 0x4460, 0x0E80, 0xC440}, // L
    };

    // Сколько шагов игры (TICK_MS) фигура висит на каждой строке, по уровням (как кадры NES,
    // но с ограничением снизу: LCD не успевает показывать слишком быстрое падение).
    const uint8_t GRAVITY[] PROGMEM = {48, 43, 38, 33, 28, 23, 18, 13, 10, 8, 7, 6, 5, 5, 4, 4};
    constexpr uint8_t GRAVITY_LEVELS = sizeof(GRAVITY);

    // Очки за 1…4 линии разом, умножаются на номер уровня (1, 2, …).
    constexpr uint16_t LINE_SCORE[4] PROGMEM = {40, 100, 300, 1200};

    // Рамка стакана: тонкая линия у правого края знакоместа (слева от поля) и у левого (справа).
    const uint8_t GLYPH_WALL_L[8] PROGMEM = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
    const uint8_t GLYPH_WALL_R[8] PROGMEM = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10};

    const Note SND_START[] PROGMEM = {
        {659, 90}, {494, 45}, {523, 45}, {587, 90}, {523, 45}, {494, 45}, {440, 180}, {0, 0}
    };
    constexpr Note SND_MOVE[] PROGMEM = {{1760, 4}, {0, 0}};
    constexpr Note SND_ROTATE[] PROGMEM = {{880, 18}, {1175, 22}, {0, 0}};
    constexpr Note SND_LOCK[] PROGMEM = {{523, 25}, {0, 0}};
    constexpr Note SND_LINE[] PROGMEM = {{784, 60}, {1047, 60}, {1319, 100}, {0, 0}};
    const Note SND_TETRIS[] PROGMEM = {
        {784, 60}, {988, 60}, {1175, 60}, {1568, 60}, {1175, 60}, {1568, 60},
        {2093, 180}, {0, 0}
    };
    const Note SND_LEVEL[] PROGMEM = {{1047, 70}, {1319, 70}, {1568, 70}, {2093, 160}, {0, 0}};
    const Note SND_GAME_OVER[] PROGMEM = {{784, 160}, {659, 160}, {523, 160}, {494, 160}, {440, 450}, {0, 0}};
} // namespace tetris
