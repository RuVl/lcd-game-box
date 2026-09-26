// Dino Run — бегущий динозавр из Chrome на две строки.
//
// Динозавр бежит во второй колонке, навстречу едут группы кактусов (1–3 маленьких, 1–2 больших)
// и птеродактили (со 150 очков). Счёт — пройденное расстояние, скорость растёт. Кактусы и низких
// птиц перепрыгивать, под средней птицей пригнуться, под высокой (в верхней строке) — не прыгать.
//
// Управление: ↑ или нажатие стика — прыжок (удержание — прыгать снова при приземлении; из приседа
// тоже прыжок), ↓ — пригнуться, ↓, нажатая в прыжке, — быстрое падение (↓, зажатая ещё до
// прыжка, его не включает). Титульный экран: нажатие — старт, удержание стика — выход в меню.
//
// Плавное движение: поле рисуется по точкам. Каждая клетка экрана — 6 точек в ширину
// (5 видимых + промежуток между символами), препятствия сдвигаются на 1 точку. Занятые
// клетки собираются в символы CGRAM на лету. Бюджет 8 символов: динозавр — до 2 клеток, группа
// (не шире 11 точек, в одной строке) — до 3, птица — до 2. Просвет между группами не меньше
// 36 точек, поэтому на экране одновременно не больше 6 клеток препятствий (перебор всех
// сочетаний групп и сдвигов: при просвете ≥ 34 максимум ровно 8 вместе с динозавром).
// Столкновение — по точкам: спрайты пересеклись в одной клетке.
#pragma once

#include "core/Game.h"
#include "core/GameScreens.h"
#include "core/Phase.h"

class DinoGame : public Game {
 public:
  void begin() override;
  void update() override;

 private:
  enum class State : uint8_t { Title, Playing, Over };

  static constexpr uint8_t MAX_OBS = 4;
  static constexpr uint8_t MAX_CELLS = 8;  // столько символов в CGRAM

  struct Obstacle {
    int16_t x;  // левая точка в «точках поля» (клетка × 6)
    uint8_t type;
  };
  struct Cell {
    uint8_t pos;  // строка × 16 + колонка
    uint8_t bits[8];
  };

  // --- игра ---
  Obstacle obs[MAX_OBS];
  uint8_t obsCount;
  int16_t spawnIn;     // через сколько точек появится следующее препятствие
  uint16_t speed;      // точек за шаг × 256
  uint16_t scrollAcc;  // дробная часть сдвига
  uint8_t dotAcc;      // точки, ещё не ставшие очками
  uint32_t score;
  uint8_t hundred;     // очки сверх последней полной сотни
  uint8_t flashTicks;  // счёт мигает после очередной сотни
  uint8_t jumpT;       // 0 — на земле, иначе шаг прыжка 1…JUMP_LEN
  bool ducking;
  bool downLocked;     // ↓ зажата с момента прыжка: быстрого падения нет, пока не отпустят
  bool jumpLatched;
  uint16_t tickCount;
  uint32_t nextTick;

  // --- отрисовка ---
  Cell cells[MAX_CELLS];
  uint8_t cellCount;
  uint8_t slotBits[MAX_CELLS][8];  // что сейчас в символах CGRAM

  // --- автомат состояний ---
  State state;
  Phase phase;
  TitleScreen title;
  ResultScreen result;

  uint8_t *cellRows(uint8_t pos);
  bool plot(int8_t col, uint8_t y, uint8_t bits);
  bool drawSprite(uint8_t spr, int16_t x, uint8_t y);
  bool render(bool dinoVisible = true);  // true — динозавр задел препятствие
  void flushCells();

  void spawn();
  bool gameTick();

  void enter(State s);
  void startPlaying();
  void updateTitle();
  void updatePlaying();
  void updateOver();
};
