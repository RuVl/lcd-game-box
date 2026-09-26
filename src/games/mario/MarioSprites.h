// Mario: спрайты 5×8 в PROGMEM. Подключается только из MarioGame.cpp.
#pragma once

#include "games/mario/MarioTypes.h"

namespace mario
{
    const uint8_t SPR_GROUND[8] PROGMEM = {0, 0, 0, 0, 0, 0, 0b11111, 0b10101};
    // Кирпичи со смещённой кладкой: ряд подряд читается как одна стена.
    const uint8_t SPR_BRICK[8] PROGMEM = {0b11111, 0b11111, 0, 0b11011, 0b11011, 0, 0b11111, 0b11111};
    const uint8_t SPR_USED[8] PROGMEM = {0b11111, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11111};
    const uint8_t SPR_QBLOCK[8] PROGMEM = {0b11111, 0b10001, 0b11101, 0b11011, 0b11011, 0b11111, 0b11011, 0b11111};
    const uint8_t SPR_COIN_A[8] PROGMEM = {0, 0b01110, 0b11101, 0b11101, 0b11101, 0b11101, 0b01110, 0};
    const uint8_t SPR_COIN_B[8] PROGMEM = {0, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0};
    const uint8_t SPR_MUSHROOM[8] PROGMEM = {0, 0b01110, 0b11011, 0b11111, 0b11111, 0b01010, 0b01110, 0};
    const uint8_t SPR_GOOMBA_A[8] PROGMEM = {0, 0b01110, 0b11111, 0b10101, 0b11111, 0b01010, 0b11111, 0b10101};
    const uint8_t SPR_GOOMBA_B[8] PROGMEM = {0, 0b01110, 0b11111, 0b10101, 0b11111, 0b10001, 0b11111, 0b10101};
    const uint8_t SPR_KOOPA[8] PROGMEM = {0b01000, 0b11000, 0b01110, 0b11111, 0b11111, 0b01010, 0b11111, 0b10101};
    const uint8_t SPR_SHELL[8] PROGMEM = {0, 0, 0, 0b01110, 0b10101, 0b11111, 0b11111, 0b10101};
    const uint8_t SPR_PIPE[8] PROGMEM = {0b11111, 0b10001, 0b11111, 0b01010, 0b01010, 0b01010, 0b01010, 0b01010};
    const uint8_t SPR_PLANT_A[8] PROGMEM = {0b01110, 0b11011, 0b11111, 0b01110, 0b00100, 0b10101, 0b01110, 0b00100};
    const uint8_t SPR_PLANT_B[8] PROGMEM = {0b01110, 0b11111, 0b11111, 0b01110, 0b00100, 0b10101, 0b01110, 0b00100};
    const uint8_t SPR_LIFT[8] PROGMEM = {0, 0, 0, 0, 0, 0b11111, 0b10001, 0b11111};
    const uint8_t SPR_SPRING[8] PROGMEM = {0, 0, 0b11111, 0b01010, 0b00100, 0b01010, 0b11111, 0b10101};
    const uint8_t SPR_FIRE_A[8] PROGMEM = {0, 0b00100, 0b01110, 0b11111, 0b11111, 0b01110, 0b00100, 0};
    const uint8_t SPR_FIRE_B[8] PROGMEM = {0, 0b01010, 0b00100, 0b01110, 0b11111, 0b01110, 0b00100, 0};
    const uint8_t SPR_LAVA_A[8] PROGMEM = {0, 0, 0, 0, 0, 0b01001, 0b10110, 0b11111};
    const uint8_t SPR_LAVA_B[8] PROGMEM = {0, 0, 0, 0, 0, 0b10010, 0b01101, 0b11111};
    const uint8_t SPR_BOWSER[8] PROGMEM = {0b10001, 0b11111, 0b10101, 0b11111, 0b01110, 0b11111, 0b11111, 0b10101};
    const uint8_t SPR_AXE[8] PROGMEM = {0b01110, 0b11111, 0b11111, 0b01110, 0b00100, 0b00100, 0b11111, 0b10101};

    // Кадр A и кадр B (для анимированных; nullptr - без анимации). Порядок как в Spr.
    const uint8_t* const SPRITE_A[S_COUNT] PROGMEM = {
        SPR_GROUND, SPR_BRICK, SPR_USED, SPR_QBLOCK, SPR_COIN_A, SPR_MUSHROOM, SPR_GOOMBA_A, SPR_KOOPA, SPR_SHELL,
        SPR_PIPE, SPR_PLANT_A, SPR_LIFT, SPR_SPRING, SPR_FIRE_A, SPR_LAVA_A, SPR_BOWSER, SPR_AXE
    };
    const uint8_t* const SPRITE_B[S_COUNT] PROGMEM = {
        nullptr, nullptr, nullptr, nullptr, SPR_COIN_B, nullptr, SPR_GOOMBA_B, nullptr, nullptr,
        nullptr, SPR_PLANT_B, nullptr, nullptr, SPR_FIRE_B, SPR_LAVA_B, nullptr, nullptr
    };

    // Марио: маленький (с полоской земли под ногами) и большой (во всю высоту клетки).
    const uint8_t SPR_MARIO[8] PROGMEM = {0b01110, 0b01111, 0b01110, 0b11111, 0b01110, 0b01010, 0b11111, 0b10101};
    const uint8_t SPR_MARIO_RUN[8] PROGMEM = {0b01110, 0b01111, 0b01110, 0b11110, 0b01110, 0b10001, 0b11111, 0b10101};
    const uint8_t SPR_MARIO_AIR[8] PROGMEM = {0, 0b01110, 0b01111, 0b01110, 0b11101, 0b01110, 0b10010, 0};
    const uint8_t SPR_BIG[8] PROGMEM = {0b01110, 0b01111, 0b01110, 0b11111, 0b11111, 0b11111, 0b01010, 0b11011};
    const uint8_t SPR_BIG_RUN[8] PROGMEM = {0b01110, 0b01111, 0b01110, 0b11110, 0b11111, 0b11111, 0b10001, 0b10001};
    const uint8_t SPR_BIG_AIR[8] PROGMEM = {0b01110, 0b01111, 0b01110, 0b11101, 0b11111, 0b11111, 0b10010, 0};

    // Только для финального экрана.
    const uint8_t SPR_PRINCESS[8] PROGMEM = {0b10101, 0b11111, 0b01110, 0b00100, 0b01110, 0b11111, 0b11111, 0b10101};
    const uint8_t SPR_HEART[8] PROGMEM = {0, 0b01010, 0b11111, 0b11111, 0b01110, 0b00100, 0, 0};
} // namespace mario
