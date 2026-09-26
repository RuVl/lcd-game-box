// Mario: константы и типы, общие для класса игры и файлов с данными.
#pragma once

#include <Arduino.h>

namespace mario {

constexpr uint16_t TICK_MS = 140;      // один шаг игры
constexpr uint8_t AIR_TICKS = 3;       // сколько шагов Марио висит в воздухе
constexpr uint8_t SPRING_AIR = 7;      // прыжок с пружины: перелетает яму до 8 клеток
constexpr uint8_t INVULN_TICKS = 14;   // неуязвимость после потери гриба (~2 с)
constexpr uint8_t BOWSER_EVERY = 2;    // Боузер действует раз в столько шагов
constexpr uint8_t BOWSER_FIRE_EVERY = 12;
constexpr uint8_t LIFT_EVERY = 2;      // платформа сдвигается раз в столько шагов
constexpr uint8_t PLANT_PERIOD = 30;   // пиранья: цикл и сколько шагов из него она снаружи
constexpr uint8_t PLANT_UP = 12;
constexpr uint8_t POD_PERIOD = 20;     // огненный шар из лавы: цикл прыжка
constexpr uint8_t START_LIVES = 3;
constexpr uint8_t MAX_LIVES = 9;
constexpr uint8_t COINS_PER_1UP = 30;
constexpr uint8_t CAM_LEAD = 5;        // колонка экрана, дальше которой камера едет за Марио
constexpr uint8_t POP_TICKS = 4;       // сколько шагов монета/гриб видны в выбитом блоке

constexpr uint8_t MAX_LEVEL_LEN = 160;
constexpr uint8_t MAX_ENEMIES = 14;
constexpr uint8_t MAX_LIFTS = 4;
constexpr uint8_t MAX_FLAMES = 3;


constexpr uint8_t HISCORE_SLOT = 0;  // слот рекорда в EEPROM

struct LevelDef {
  const char *top;
  const char *bottom;
  uint8_t world, num;
  uint8_t enemyEvery;  // гумбы и купы ходят раз в столько шагов
  bool castle;
  bool bowserFire;     // настоящий Боузер плюётся огнём
};

// Спрайты уровня. Каждому уровню раздаются только нужные ему (не больше 7).
enum Spr : uint8_t {
  S_GROUND, S_BRICK, S_USED, S_QBLOCK, S_COIN, S_MUSHROOM, S_GOOMBA, S_KOOPA, S_SHELL,
  S_PIPE, S_PLANT, S_LIFT, S_SPRING, S_FIRE, S_LAVA, S_BOWSER, S_AXE, S_COUNT
};

enum MarioPose : uint8_t { POSE_STAND, POSE_RUN, POSE_AIR };

enum EnemyType : uint8_t { E_NONE, E_GOOMBA, E_KOOPA, E_SHELL, E_SHELL_MOVING };

struct Enemy {
  uint8_t x;
  int8_t dir;
  uint8_t type;
};

// Платформа ездит туда-обратно по своей яме. Положение считается из номера шага,
// поэтому хранить его не нужно, и откат чита возвращает платформы сам собой.
struct Lift {
  uint8_t lo, hi;  // края ямы
  uint8_t w;       // ширина платформы
  uint8_t o0;      // начальное смещение от lo
};

struct Bowser {
  uint8_t x, y;
  int8_t dir;
  uint8_t air;
  bool alive;
};

struct Flame {  // огонь Боузера летит влево по верхнему ряду
  uint8_t x;
  bool active;
};


}  // namespace mario
