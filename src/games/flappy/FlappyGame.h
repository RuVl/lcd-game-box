// Flappy Bird в пиксельном режиме: холст 4×2 знакоместа (20×16 точек) слева, счёт справа.
//
// Птица (2×2 точки) висит в колонке у левого края, трубы едут справа налево.
// ↑ или нажатие стика — взмах. Игра начинается с первого взмаха, до этого птица парит.
// За каждую пройденную трубу +1 очко. Удар о трубу или пол — конец игры, в потолок можно упереться.
// Физика и размеры — как в оригинале: скорость постоянная, проём ≈ 4 высоты птицы.
// Титульный экран: нажатие — старт, удержание стика — выход в меню устройства.
#pragma once

#include "core/Game.h"
#include "core/GameScreens.h"
#include "core/Phase.h"
#include "core/PixelCanvas.h"

class FlappyGame : public Game {
 public:
  void begin() override;
  void update() override;

 private:
  enum class State : uint8_t { Title, Playing, Crash, GameOver };

  struct Pipe {
    int16_t x;     // левый край, 1/16 точки (может уйти за левый край холста)
    uint8_t gapY;  // верх проёма, точки
    bool passed;   // очко за неё уже начислено
  };
  static constexpr uint8_t PIPE_COUNT = 2;

  PixelCanvas canvas;
  Pipe pipes[PIPE_COUNT];
  int16_t birdY;   // верх птицы, 1/16 точки
  int8_t birdVy;   // скорость, 1/16 точки за шаг (плюс — вниз)
  uint16_t score;
  uint32_t nextTick;
  bool started;      // был первый взмах: до него птица парит, трубы стоят
  bool flapLatched;  // нажатие между шагами не теряется

  State state;
  Phase phase;
  TitleScreen title;
  ResultScreen result;

  void enter(State s);
  void resetRound();
  void placePipe(Pipe &p, int16_t x);
  bool tick(bool flap);  // шаг физики; false — птица разбилась
  bool hitsPipe() const;
  void render(bool birdVisible = true);
  void showScore();

  void updateTitle();
  void startRound();
  void updatePlaying();
  void updateCrash();
  void updateGameOver();
};
