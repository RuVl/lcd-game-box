// Змейка на пиксельном холсте 4×2 знакоместа (20×16 точек), одна точка — один сегмент.
//
// Цель: съесть как можно больше еды (мигающая точка). Змейка растёт на сегмент за каждую еду
// и понемногу ускоряется; очки за еду растут вместе со скоростью.
//
// Управление: джойстик — поворот (разворот на 180° игнорируется, один поворот запоминается
// наперёд, чтобы быстрые «змейки» из двух поворотов не терялись). Зажатая кнопка стика —
// змейка ползёт вдвое быстрее.
// Титульный экран: ←/→ — режим (WALLS: стены убивают; WRAP: сквозь края), нажатие — старт,
// удержание стика — выход в меню устройства.
#pragma once

#include "core/Game.h"
#include "core/GameScreens.h"
#include "core/Phase.h"
#include "core/PixelCanvas.h"

class SnakeGame : public Game {
 public:
  void begin() override;
  void update() override;

 private:
  enum class State : uint8_t { Title, Playing, Dying, Final };

  static constexpr uint8_t W = 20, H = 16;
  static constexpr uint16_t CELLS = W * H;

  // Мелкие поля — в начале объекта: на AVR к ним короткий доступ (смещение < 64),
  // большие массивы — в конце.
  State state;
  Phase phase;
  bool wrap;  // режим «сквозь края»; выбирается на титульном экране и сохраняется между играми
  bool won;   // змейка заняла всё поле

  uint16_t tailIdx;  // индекс шага, начинающегося в хвосте
  uint16_t steps;    // шагов в буфере = длина − 1
  uint8_t headX, headY, tailX, tailY;
  uint8_t dir;       // текущее направление: 0 →, 1 ↓, 2 ←, 3 ↑
  uint8_t turns[2];  // очередь поворотов
  uint8_t turnCount;
  uint8_t lastInput;  // направление стика на прошлом проходе (повороты — по его смене)

  uint8_t foodX, foodY;
  bool hasFood;

  uint16_t eaten;
  uint16_t score;
  uint32_t nextTick;

  TitleScreen title;
  // Тело: кольцевой буфер направлений по 2 бита — шаги от хвоста к голове.
  uint8_t dirs[CELLS / 4];
  PixelCanvas canvas;  // поле; сама картинка служит и картой занятых клеток

  uint8_t getDir(uint16_t i) const;
  void setDir(uint16_t i, uint8_t d);
  void spawnFood();
  void queueTurn(uint8_t d);
  void renderStatus();
  const char *modeLine() const;
  bool step();  // false — змейка разбилась

  void enter(State s);
  void startPlaying();
  void updateTitle();
  void updatePlaying();
  void updateDying();
  void updateFinal();
};
