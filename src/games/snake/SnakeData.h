// Snake: мелодии в PROGMEM. Подключается только из SnakeGame.cpp.
#pragma once

#include "core/Sound.h"

namespace snake
{
    const Note SND_START[] PROGMEM = {{523, 80}, {659, 80}, {784, 80}, {1047, 160}, {0, 0}};
    constexpr Note SND_EAT[] PROGMEM = {{1047, 30}, {1568, 50}, {0, 0}};
    constexpr Note SND_MODE[] PROGMEM = {{880, 40}, {0, 0}};
    const Note SND_OVER[] PROGMEM = {{784, 150}, {659, 150}, {523, 150}, {440, 400}, {0, 0}};
    const Note SND_WIN[] PROGMEM = {{523, 100}, {659, 100}, {784, 100}, {1047, 200}, {784, 100}, {1047, 400}, {0, 0}};
} // namespace snake
