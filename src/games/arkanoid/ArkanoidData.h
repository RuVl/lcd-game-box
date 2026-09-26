// Arkanoid: уровни, направления мяча и мелодии в PROGMEM. Подключается только из ArkanoidGame.cpp.
#pragma once

#include "core/Sound.h"
#include "games/arkanoid/ArkanoidGame.h"

namespace arkanoid {

// Уровни: BRICK_ROWS строк по BRICK_COLS кирпичей, бит 9 — левый кирпич (точки x 0…1).
const uint16_t LAYOUTS[][BRICK_ROWS] PROGMEM = {
    {  // стена
     0b1111111111,
     0b1111111111,
     0b1111111111,
     0b0000000000,
     0b0000000000,
     0b0000000000},
    {  // пирамида
     0b0000110000,
     0b0001111000,
     0b0011111100,
     0b0111111110,
     0b1111111111,
     0b0000000000},
    {  // шахматы
     0b1010101010,
     0b0101010101,
     0b1010101010,
     0b0101010101,
     0b1010101010,
     0b0101010101},
    {  // крепость: середину закрывает кольцо
     0b0111111110,
     0b1100000011,
     0b1001111001,
     0b1001111001,
     0b1100000011,
     0b0111111110},
};
constexpr uint8_t LAYOUT_COUNT = sizeof(LAYOUTS) / sizeof(LAYOUTS[0]);

// Направления отскока от ракетки, длина вектора ≈ 64: углы от вертикали −60°, −40°, −20°,
// 20°, 40°, 60°. Край ракетки даёт пологий отскок, середина — крутой.
const int8_t DIR_X[] PROGMEM = {-55, -41, -22, 22, 41, 55};
const int8_t DIR_Y[] PROGMEM = {32, 49, 60, 60, 49, 32};
constexpr uint8_t DIR_COUNT = sizeof(DIR_X);

const Note SND_START[] PROGMEM = {{523, 80}, {659, 80}, {784, 80}, {1047, 160}, {0, 0}};
const Note SND_LAUNCH[] PROGMEM = {{1047, 30}, {1568, 40}, {0, 0}};
const Note SND_PADDLE[] PROGMEM = {{784, 30}, {0, 0}};
const Note SND_BRICK[] PROGMEM = {{1568, 25}, {0, 0}};
const Note SND_LOST[] PROGMEM = {{784, 120}, {659, 120}, {523, 120}, {440, 300}, {0, 0}};
const Note SND_CLEAR[] PROGMEM = {{1047, 90}, {1319, 90}, {1568, 90}, {2093, 90}, {1, 60}, {1568, 90},
                                  {2093, 300}, {0, 0}};
const Note SND_GAMEOVER[] PROGMEM = {{659, 220}, {622, 220}, {587, 220}, {554, 220}, {523, 260},
                                     {440, 500}, {0, 0}};

}  // namespace arkanoid
