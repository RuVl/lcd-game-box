// LCD Game Box: игры для LCD1602 + джойстика + пассивного зуммера на Arduino UNO или ESP8266.
// Подключение железа — в core/Config.h, список игр — в games/Registry.cpp.
#include <Arduino.h>

#include "core/Display.h"
#include "core/Input.h"
#include "core/Launcher.h"
#include "core/Network.h"
#include "core/Sound.h"
#include "core/Storage.h"

void setup() {
  Storage::begin();
  Input::begin();
  Sound::begin();
  Display::begin();
#if GAMEBOX_HAS_WIFI
  Network::begin();
#endif
  Launcher::begin();
}

void loop() {
  Sound::update();
  Input::update();
#if GAMEBOX_HAS_WIFI
  Network::update();
#endif
  Launcher::update();
}
