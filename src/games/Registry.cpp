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
#elif defined(GAMEBOX_UNO_ARCADE)
#define WITH_MARIO 0
#define WITH_TETRIS 1
#else
#define WITH_MARIO 1
#define WITH_TETRIS 1
#endif

#if WITH_MARIO
#include "games/mario/MarioGame.h"
#endif
#if WITH_TETRIS
#include "games/tetris/TetrisGame.h"
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
    // Служебное — в конце списка
    {"JOYSTICK SETUP", create<JoystickSetup>},
#if GAMEBOX_HAS_WIFI
    {"SETTINGS", create<SettingsApp>},
#endif
};
const uint8_t GAME_COUNT = sizeof(GAMES) / sizeof(GAMES[0]);
