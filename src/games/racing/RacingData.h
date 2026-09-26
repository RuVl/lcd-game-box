// Гонка: константы, символы и мелодии в PROGMEM. Подключается только из RacingGame.cpp.
#pragma once

#include "core/Sound.h"

namespace racing
{
    constexpr uint16_t TICK_MS = 20; // шаг игры (50 в секунду)
    constexpr uint8_t LANES = 4;
    constexpr uint8_t PLAYER_COL = 1;
    constexpr uint8_t HUD_COL = ROAD_COLS;
    constexpr uint8_t MAX_GEAR = 6;
    constexpr uint8_t ACCEL_TICKS = 20; // газ: +1 передача раз в 0,4 с
    constexpr uint8_t DECEL_TICKS = 12; // газ отпущен: −1 передача раз в 0,24 с
    constexpr uint8_t MIN_FIRST = 50; // колонок до первого подъёма минимальной передачи (1→2)
    constexpr uint8_t MIN_GROW = 60; // каждый следующий подъём - на столько колонок дальше:
    // 50, 110, 170, 230, 290 (итого 50, 160, 330, 560, 850)
    constexpr uint8_t MARK_EVERY = 3; // разметка - в каждой третьей колонке
    constexpr uint8_t DIST_PER_LEVEL = 200; // колонок пути на уровень сложности
    constexpr uint8_t NEAR_MISS_BONUS = 5; // × передача
    constexpr uint8_t HISCORE_SLOT = 7; // слот рекорда в EEPROM

    // Скорость разметки (1/256 колонки за шаг) для передач 1…6:
    // 2,9; 3,9; 5,1; 6,3; 7,4; 8,8 колонки/с. Шестая теперь доступна всегда (газом),
    // поэтому она медленнее прежних "нитро"-передач. Машины едут медленнее нас: поток
    // сдвигается на 3/4 этой скорости, поэтому разметка обгоняет машины.
    const uint8_t ROAD_RATE[MAX_GEAR] PROGMEM = {15, 20, 26, 32, 38, 45};

    // Уровни сложности: промежуток между рядами машин (минимум и случайная добавка, колонки),
    // сколько машин в ряду (до).
    struct Level
    {
        uint8_t gapMin, gapRand, maxCars;
    };

    const Level LEVELS[] PROGMEM = {
        {5, 3, 1}, {5, 3, 2}, {4, 3, 2}, {4, 2, 2}, {3, 2, 3},
        {3, 2, 3}, {2, 2, 3}, {2, 1, 3}, {2, 1, 3}, {2, 0, 3},
    };
    constexpr uint8_t MAX_LEVEL = sizeof(LEVELS) / sizeof(LEVELS[0]) - 1;

    // Символы CGRAM. Полоса - половина клетки: верхняя - строки 0…2, нижняя - 4…6.
    enum : uint8_t { G_CAR_TOP, G_CAR_BOT, G_CAR_BOTH, G_ME_TOP, G_ME_BOT, G_ME_TOP_CAR, G_ME_BOT_CAR, G_MARK };

    constexpr uint8_t GLYPH_COUNT = 8;

#define RACING_CAR 0b11110, 0b11111, 0b11110  // попутная машина: коробка с капотом
#define RACING_ME 0b11010, 0b01111, 0b11010   // игрок: болид с колёсами
    const uint8_t GLYPHS[GLYPH_COUNT][8] PROGMEM = {
        {RACING_CAR, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, RACING_CAR, 0},
        {RACING_CAR, 0, RACING_CAR, 0},
        {RACING_ME, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, RACING_ME, 0},
        {RACING_ME, 0, RACING_CAR, 0}, // игрок сверху, машина снизу
        {RACING_CAR, 0, RACING_ME, 0}, // машина сверху, игрок снизу
        {0, 0, 0, 0b11100, 0, 0, 0, 0}, // штрих разметки между полосами
    };
#undef RACING_CAR
#undef RACING_ME

    // Взрыв - на время аварии загружается в символ разметки.
    const uint8_t SPR_BOOM[8] PROGMEM = {0b00100, 0b10101, 0b01110, 0b11011, 0b01110, 0b10101, 0b00100, 0};

    const Note SND_START[] PROGMEM = {
        {880, 120}, {1, 380}, {880, 120}, {1, 380}, {880, 120}, {1, 380},
        {1760, 400}, {0, 0}
    };
    constexpr Note SND_GEAR_UP[] PROGMEM = {{660, 20}, {880, 30}, {0, 0}};
    constexpr Note SND_GEAR_DOWN[] PROGMEM = {{880, 20}, {587, 30}, {0, 0}};
    constexpr Note SND_GAS[] PROGMEM = {{523, 25}, {784, 40}, {0, 0}}; // газ нажат
    constexpr Note SND_NEAR_MISS[] PROGMEM = {{1568, 25}, {2093, 35}, {0, 0}};
    const Note SND_CRASH[] PROGMEM = {
        {1200, 40}, {600, 40}, {1000, 40}, {500, 40}, {800, 60}, {450, 80},
        {700, 60}, {400, 160}, {0, 0}
    };
    const Note SND_GAME_OVER[] PROGMEM = {{784, 160}, {659, 160}, {523, 160}, {440, 400}, {0, 0}};
    const Note SND_RECORD[] PROGMEM = {{1047, 90}, {1319, 90}, {1568, 90}, {2093, 250}, {0, 0}};
} // namespace racing
