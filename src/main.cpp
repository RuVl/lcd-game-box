// LCD Game Box: игры для LCD1602 + джойстика + пассивного зуммера на Arduino UNO или ESP8266.
// Подключение железа - в core/Config.h, список игр - в games/Registry.cpp.
#include <Arduino.h>

#include "core/Config.h"
#if !defined(GAMEBOX_HAS_WIFI)
#error "core/Config.h is required: GAMEBOX_* macros select what is built"
#endif
#include "core/Display.h"
#include "core/Input.h"
#include "core/Launcher.h"
#include "core/Sound.h"
#include "core/Storage.h"
#if GAMEBOX_HAS_WIFI
#include "core/Network.h"
#endif

void setup()
{
    Storage::begin();
    Input::begin();
    Sound::begin();
    Display::begin();
#if GAMEBOX_HAS_WIFI
    Network::begin();
#endif
    Launcher::begin();
}

void loop()
{
    Sound::update();
    Input::update();
#if GAMEBOX_HAS_WIFI
    Network::update();
#endif
    Launcher::update();
}
