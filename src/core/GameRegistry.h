// Список игр устройства. Сам список и общая область памяти - в games/Registry.cpp.
#pragma once

#include <Arduino.h>

#include "core/Game.h"

struct GameInfo
{
    const char* title; // название в меню, до 14 символов
    Game*(*create)(void* memory); // создать игру в переданной области памяти
};

extern const GameInfo GAMES[];
extern const uint8_t GAME_COUNT;

// Общая область памяти: в ней живёт только запущенная игра.
extern uint8_t gameArena[];
