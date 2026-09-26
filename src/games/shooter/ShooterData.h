// Space Shooter: половинки спрайтов, мелодии и параметры в PROGMEM. Подключается только из ShooterGame.cpp.
#pragma once

#include "core/Sound.h"

namespace shooter {

constexpr uint16_t TICK_MS = 70;  // один шаг игры
constexpr uint8_t HISCORE_SLOT = 6;
constexpr uint8_t START_LIVES = 3;
constexpr uint8_t LANES = 4;           // по две дорожки в каждой строке экрана
constexpr uint8_t SHIP_MAX_X = 3;      // корабль ходит по колонкам 0…3
constexpr uint8_t TAP_FIRE_TICKS = 2;  // пауза после выстрела нажатием
constexpr uint8_t AUTO_FIRE_TICKS = 5; // и между выстрелами автоогня
constexpr uint8_t BOOM_TICKS = 4;
constexpr uint8_t INVULN_TICKS = 30;   // неуязвимость после потери корабля
constexpr uint8_t BOSS_EVERY = 4;      // каждая 4-я волна — босс
constexpr uint8_t BOSS_X = 12;         // где босс останавливается (занимает колонки 12…13)
constexpr uint8_t WINDUP_TICKS = 8;    // босс мигает перед рывком: столько шагов, чтобы уйти с его дорожек

// Что может лежать на дорожке. Тип врага — тоже вид: SAUCER, ZIG, GUN идут подряд.
// BOLT — заряд босса: выстрелы его не сбивают, от него только уворачиваться.
enum Kind : uint8_t {
  K_NONE, K_SHIP, K_SHOT, K_SAUCER, K_ZIG, K_GUN, K_BULLET, K_BOLT, K_BOOM, K_BOSS_LT, K_BOSS_LB, K_BOSS_RT, K_BOSS_RB
};

// Половинка символа (4 строки из 8): символ клетки = верхняя дорожка + нижняя.
const uint8_t HALF[][4] PROGMEM = {
    {0, 0, 0, 0},                          // пусто
    {0b11000, 0b01111, 0b11000, 0},        // корабль
    {0, 0b01110, 0, 0},                    // выстрел
    {0b01110, 0b10101, 0b01110, 0},        // тарелка
    {0b10001, 0b01110, 0b10001, 0},        // зигзаг
    {0b00111, 0b11110, 0b00111, 0},        // пушка
    {0, 0b00110, 0b00110, 0},              // пуля врага
    {0b00100, 0b11111, 0b00100, 0},        // заряд босса
    {0b10101, 0b01010, 0b10101, 0},        // взрыв
    {0b00001, 0b00111, 0b01101, 0b11111},  // босс 10×8: левая половина, верх
    {0b11111, 0b01101, 0b00111, 0b00001},  //   левая, низ
    {0b11110, 0b11111, 0b10011, 0b11110},  //   правая, верх
    {0b11110, 0b10011, 0b11111, 0b11110},  //   правая, низ
};

// Если на экране больше разных клеток, чем символов CGRAM, — обычная буква.
const char FALLBACK[] PROGMEM = " >-ox<.+*####";

const Note SND_SHOT[] PROGMEM = {{1760, 15}, {1397, 20}, {0, 0}};
const Note SND_BOOM[] PROGMEM = {{880, 25}, {494, 25}, {698, 25}, {415, 50}, {0, 0}};
const Note SND_BOSS_HIT[] PROGMEM = {{1175, 20}, {784, 30}, {0, 0}};
const Note SND_HIT[] PROGMEM = {{1047, 60}, {784, 60}, {587, 80}, {440, 250}, {0, 0}};
const Note SND_WAVE[] PROGMEM = {{523, 80}, {659, 80}, {784, 80}, {1047, 160}, {0, 0}};
const Note SND_BOSS[] PROGMEM = {{440, 150}, {1, 60}, {440, 150}, {1, 60}, {440, 150}, {1, 60}, {880, 300}, {0, 0}};
const Note SND_BOSS_DOWN[] PROGMEM = {{1047, 60}, {523, 60}, {988, 60}, {494, 60}, {880, 60}, {440, 60},
                                      {1319, 300}, {0, 0}};
const Note SND_OVER[] PROGMEM = {{784, 200}, {659, 200}, {523, 200}, {415, 500}, {0, 0}};

}  // namespace shooter
