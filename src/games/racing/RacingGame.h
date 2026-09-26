// Гонка по шоссе: вид сверху, дорога едет справа налево, машина игрока — у левого края.
//
// Полос четыре: в каждой строке экрана две — машины рисуются в верхней или нижней
// половине клетки. Справа — счёт (пройденный путь + бонусы), передача и минимальная передача.
//
// Управление: ↑/↓ — сменить полосу. Удержание стика — газ: передача плавно растёт до 6-й
// и держится, пока стик нажат; отпустил — машина плавно сбрасывает до крейсерской.
// →/← — крейсерская передача (1…6) — ручная альтернатива газу, выше 6-й не бывает.
// Минимальная передача растёт с путём: первый раз — через 50 колонок, каждый следующий —
// всё дальше; ниже минимума передача не опускается.
// Чем быстрее едешь, тем быстрее идёт счёт. Проезд вплотную к машине — бонус.
// Сложность растёт с пройденным путём: машин больше, промежутки короче.
// Титульный экран: нажатие — старт, удержание стика — выход в меню устройства.
#pragma once

#include "core/Game.h"
#include "core/GameScreens.h"
#include "core/Phase.h"

namespace racing {
constexpr uint8_t ROAD_COLS = 12;  // колонки 12…15 — счёт и приборы
}

class RacingGame : public Game {
 public:
  void begin() override;
  void update() override;

 private:
  enum class State : uint8_t { Title, Start, Playing, Crash, GameOver };

  // Состояние заезда: перед стартом обнуляется целиком.
  struct Run {
    // --- дорога ---
    uint8_t road[racing::ROAD_COLS];  // машины в колонке: бит i — полоса i (0 — самая верхняя)
    uint8_t gapLeft;                  // пустых колонок до следующей колонки с машинами
    uint8_t prevCars;                 // машины в последней колонке, где они были
    uint8_t markPhase;                // колонка первого штриха разметки (0…2)
    uint16_t roadAcc, trafficAcc;     // доли колонки (1/256) до сдвига разметки / машин
    uint8_t level;                    // сложность 0…MAX_LEVEL
    uint8_t levelDist;                // колонок пути на текущем уровне

    // --- игрок ---
    uint8_t lane;
    uint8_t speed;     // текущая передача 1…MAX_GEAR, не ниже minGear
    uint8_t gear;      // крейсерская передача (→/←), не ниже minGear
    uint8_t minGear;   // минимальная передача
    uint8_t shiftWait; // шагов до следующей смены передачи (0 — можно сразу)
    bool gas;          // стик удерживается
    uint16_t minLeft;  // колонок пути до подъёма минимальной передачи
    uint16_t score;  // до 65535, дальше не растёт
  } run;

  // --- автомат состояний ---
  State state;
  Phase phase;
  int8_t laneLatch, speedLatch;  // нажатия между шагами не теряются
  uint32_t nextTick;
  TitleScreen title;
  ResultScreen result;

  void addScore(uint8_t points);
  uint8_t newColumn();
  bool hitsPlayer() const;
  void scrollTraffic();
  bool gameTick();  // false — авария
  void render(bool showPlayer = true);
  void renderHud();

  void enter(State s);
  void updateTitle();
  void updateStart();
  void updatePlaying();
  void updateCrash();
  void updateGameOver();
};
