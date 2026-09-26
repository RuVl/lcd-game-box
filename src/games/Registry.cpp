// Список игр устройства и общая область памяти для них.
//
// Какие игры попадают в прошивку:
//   ESP8266 — все;
//   UNO (32 КБ Flash — все сразу не влезают), набор задаёт окружение сборки:
//     uno        — Mario, Tetris, Snake, Dino Run;
//     uno_arcade — все аркады без Mario (-D GAMEBOX_UNO_ARCADE).
//
// Чтобы добавить игру:
//   1. создать src/games/<name>/ с классом, унаследованным от Game (см. core/Game.h);
//   2. завести макрос WITH_<NAME> ниже, подключить заголовок, добавить sizeof в GAME_ARENA_SIZE
//      и строку в GAMES — всё под #if WITH_<NAME>;
//   3. если игра хранит рекорд — взять следующий свободный слот Storage:
//      0 Mario, 1 Tetris, 2 Snake, 3 Dino, 4 Arkanoid, 5 Flappy, 6 Shooter, 7 Racing.
#include <new>

#include "core/Config.h"
#include "core/GameRegistry.h"

#if !GAMEBOX_SMALL
#define WITH_MARIO 1
#define WITH_TETRIS 1
#define WITH_SNAKE 1
#define WITH_DINO 1
#define WITH_ARKANOID 1
#define WITH_FLAPPY 1
#define WITH_SHOOTER 1
#define WITH_RACING 1
#elif defined(GAMEBOX_UNO_ARCADE)
#define WITH_MARIO 0
#define WITH_TETRIS 1
#define WITH_SNAKE 1
#define WITH_DINO 1
#define WITH_ARKANOID 1
#define WITH_FLAPPY 1
#define WITH_SHOOTER 1
#define WITH_RACING 1
#else
#define WITH_MARIO 1
#define WITH_TETRIS 1
#define WITH_SNAKE 1
#define WITH_DINO 1
#define WITH_ARKANOID 0
#define WITH_FLAPPY 0
#define WITH_SHOOTER 0
#define WITH_RACING 0
#endif

#if WITH_MARIO
#include "games/mario/MarioGame.h"
#endif
#if WITH_TETRIS
#include "games/tetris/TetrisGame.h"
#endif
#if WITH_SNAKE
#include "games/snake/SnakeGame.h"
#endif
#if WITH_DINO
#include "games/dino/DinoGame.h"
#endif
#if WITH_ARKANOID
#include "games/arkanoid/ArkanoidGame.h"
#endif
#if WITH_FLAPPY
#include "games/flappy/FlappyGame.h"
#endif
#if WITH_SHOOTER
#include "games/shooter/ShooterGame.h"
#endif
#if WITH_RACING
#include "games/racing/RacingGame.h"
#endif
#include "system/JoystickSetup.h"
#include "system/SettingsApp.h"

namespace {
constexpr size_t maxOf(size_t a) { return a; }
template <class... Rest>
constexpr size_t maxOf(size_t a, size_t b, Rest... rest) {
  return maxOf(a > b ? a : b, rest...);
}

template <class T>
Game *create(void *memory) {
  return new (memory) T();
}
}  // namespace

// Размер самой большой игры: одновременно в памяти только запущенная.
constexpr size_t GAME_ARENA_SIZE = maxOf(sizeof(JoystickSetup)
#if WITH_MARIO
                                         , sizeof(MarioGame)
#endif
#if WITH_TETRIS
                                         , sizeof(TetrisGame)
#endif
#if WITH_SNAKE
                                         , sizeof(SnakeGame)
#endif
#if WITH_DINO
                                         , sizeof(DinoGame)
#endif
#if WITH_ARKANOID
                                         , sizeof(ArkanoidGame)
#endif
#if WITH_FLAPPY
                                         , sizeof(FlappyGame)
#endif
#if WITH_SHOOTER
                                         , sizeof(ShooterGame)
#endif
#if WITH_RACING
                                         , sizeof(RacingGame)
#endif
#if GAMEBOX_HAS_WIFI
                                         , sizeof(SettingsApp)
#endif
);

uint8_t gameArena[GAME_ARENA_SIZE];

const GameInfo GAMES[] = {
#if WITH_MARIO
    {"SUPER MARIO", create<MarioGame>},
#endif
#if WITH_TETRIS
    {"TETRIS", create<TetrisGame>},
#endif
#if WITH_SNAKE
    {"SNAKE", create<SnakeGame>},
#endif
#if WITH_DINO
    {"DINO RUN", create<DinoGame>},
#endif
#if WITH_ARKANOID
    {"ARKANOID", create<ArkanoidGame>},
#endif
#if WITH_FLAPPY
    {"FLAPPY BIRD", create<FlappyGame>},
#endif
#if WITH_SHOOTER
    {"SPACE SHOOTER", create<ShooterGame>},
#endif
#if WITH_RACING
    {"RACING", create<RacingGame>},
#endif
    // Служебное — в конце списка
    {"JOYSTICK SETUP", create<JoystickSetup>},
#if GAMEBOX_HAS_WIFI
    {"SETTINGS", create<SettingsApp>},
#endif
};
const uint8_t GAME_COUNT = sizeof(GAMES) / sizeof(GAMES[0]);
