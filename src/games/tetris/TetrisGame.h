// Тетрис: классический вертикальный стакан 10×16 клеток, одна клетка — одна точка LCD.
//
// Экран:  LV1 ▕··▏ 000120      поле — 2×2 знакоместа (CGRAM 0…3),
//          ▀▀  ▕··▏ L 3         рамка — CGRAM 4…5, следующая фигура — CGRAM 6…7.
//
// Управление: ←/→ — сдвиг (с автоповтором), ↓ — ускоренное падение (пока держат),
// ↑ или нажатие стика — поворот по часовой стрелке.
// Каждые 10 линий — новый уровень, фигуры падают быстрее.
// Титульный экран: нажатие — старт, удержание стика — выход в меню устройства.
#pragma once

#include "core/Game.h"
#include "core/GameScreens.h"
#include "core/Phase.h"

class TetrisGame : public Game {
 public:
  void begin() override;
  void update() override;

 private:
  enum class State : uint8_t { Title, Playing, Clearing, Topping, Result };

  static constexpr uint8_t W = 10, H = 16;

  // Сначала мелкие поля, массивы — в конце: на AVR к полям со смещением < 64 байт
  // обращение короче, это заметно экономит Flash.

  // --- падающая фигура ---
  uint8_t piece, rot, next;
  int8_t px, py;          // левый верхний угол квадрата 4×4 фигуры (py < 0 — выше стакана)
  uint8_t bag;            // «мешок» из 7 фигур: биты уже выданных
  uint8_t fallTicks;      // шагов с последнего падения на строку
  bool softDropBlocked;   // ↓ зажат с прошлой фигуры: ждать отпускания

  uint32_t score;
  uint16_t lines;
  uint8_t level;      // с 0; на экране — level + 1
  uint16_t fullRows;  // заполненные строки (мигают перед удалением)

  // --- автомат состояний ---
  State state;
  Phase phase;
  uint8_t pending;  // нажатия, накопленные до следующего шага игры
  uint32_t nextTick;

  // Стакан: строка y (0 — верх), клетка x — бит 12 − x. Биты 13…15 и 0…2 — «стены»,
  // поэтому выход фигуры за край проверяется той же маской, что и столкновение.
  uint16_t rows[H];
  uint8_t shown[4][8];  // что сейчас загружено в символы поля (CGRAM 0…3)
  TitleScreen title;
  ResultScreen result;

  uint16_t shape(uint8_t rotation) const;
  bool fits(uint8_t rotation, int8_t x, int8_t y) const;
  uint16_t pieceRow(int8_t y) const;
  uint8_t takeFromBag();
  bool spawn();
  bool tryMove(int8_t dx, int8_t dy);
  void rotate();
  void lock();
  void addScore(uint32_t points);
  void removeFullRows();

  void render(bool withPiece = true);
  void drawNext();
  void drawStats();

  void enter(State s);
  void startGame();
  void gameTick();
  void updateTitle();
  void updatePlaying();
  void updateClearing();
  void updateTopping();
  void updateResult();
};
