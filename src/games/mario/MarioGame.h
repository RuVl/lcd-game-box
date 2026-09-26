// Mario-платформер на две строки.
//
// Цель: пройти два мира (1-1 … 1-4 и 2-1 … 2-5). В замке 1-4 Боузер ненастоящий,
// принцесса ждёт в последнем замке 2-5.
//
// Управление: ←/→ — ходьба, ↑ или нажатие стика — прыжок.
// Титульный экран: нажатие — старт, удержание стика — выход в меню устройства.
#pragma once

#include "core/Game.h"
#include "core/GameScreens.h"
#include "core/Phase.h"
#include "games/mario/MarioTypes.h"

class MarioGame : public Game {
 public:
  void begin() override;
  void update() override;

 private:
  enum class Outcome : uint8_t { Playing, Died, Cleared };
  enum class State : uint8_t {
    Title, StartJingle, World, Playing, Dying, BridgeCollapse, LevelClear, AnotherCastle, Ending, Final
  };

  // --- уровень ---
  mario::LevelDef level;
  uint8_t levelIdx;
  uint8_t levelLen;
  char topRow[mario::MAX_LEVEL_LEN];     // изменяемая копия верхнего ряда (монеты и блоки исчезают)
  char bottomRow[mario::MAX_LEVEL_LEN];  // без врагов и платформ: они хранятся отдельно
  mario::Enemy enemies[mario::MAX_ENEMIES];
  uint8_t enemyCount;
  mario::Lift lifts[mario::MAX_LIFTS];
  uint8_t liftCount;
  mario::Bowser bowser;
  mario::Flame flames[mario::MAX_FLAMES];
  uint8_t goalX;  // флагшток или топор

  // --- Марио ---
  uint8_t marioX, marioY;  // Y: 0 — верхний ряд, 1 — нижний
  uint8_t airTicks;
  bool faceLeft;
  bool runFrame;
  bool big;        // съел гриб: переживает одно касание врага
  uint8_t invuln;  // шаги неуязвимости после потери гриба
  uint8_t camX;
  uint8_t lives;
  uint16_t coins;
  uint32_t score;
  uint32_t tickCount;
  uint8_t popX, popTicks;  // монета/гриб, вылетающие из блока

  // --- спрайты ---
  uint8_t slotOf[mario::S_COUNT];  // в каком символе CGRAM лежит спрайт
  uint8_t marioGlyphState;         // что загружено в символ Марио
  bool anim;

  // --- автомат состояний ---
  State state;
  Phase phase;
  bool jumpLatched;  // нажатие прыжка между шагами не теряется
  uint32_t nextTick;
  int16_t bridgeCell;  // какую клетку моста рушить следующей
  bool won;
  TitleScreen title;
  ResultScreen result;


  // Спрайты и отрисовка
  void loadLevelSprites(uint32_t mask);
  void animateSprites();
  char glyph(mario::Spr s) const;
  void setMarioGlyph(mario::MarioPose pose, bool left, bool isBig);
  void render(bool marioVisible = true);

  // Уровень и мир
  uint32_t usedSprites() const;
  void resetLevel();
  uint8_t liftLeft(const mario::Lift &l, uint32_t t) const;
  bool liftAt(uint8_t x) const;
  bool solidAt(uint8_t x) const;
  bool plantUp(uint8_t x) const;
  int8_t podRow(uint8_t x) const;
  int8_t enemyAt(uint8_t x, int8_t except = -1) const;
  bool onPipe() const;

  // Марио
  void addCoin();
  void collectCoinAt(uint8_t x);
  void finishPop();
  void hitBlock(uint8_t x);
  bool hurt();

  // Враги
  void killEnemy(int8_t e);
  bool kickShell(int8_t e, int8_t dir);
  void stompEnemy(int8_t e);
  void moveShells();
  void moveWalkers();
  void moveBowser();
  void moveFlames();
  bool bowserHitsMario() const;
  bool flameHitsMario() const;
  bool marioTouchesDanger() const;

  Outcome gameTick(int8_t dx, bool jumpPressed);


  // Состояния
  void enter(State s);
  void startPlaying();
  void resultScreen(const char *title);
  void formatCoinsScore(char *buf) const;
  void updateTitle();
  void updateStartJingle();
  void updateWorld();
  void updatePlaying();
  void updateDying();
  void updateBridgeCollapse();
  void updateLevelClear();
  void updateAnotherCastle();
  void updateEnding();
  void updateFinal();
};
