// Wi-Fi и прошивка по воздуху (только платы с Wi-Fi, сейчас ESP8266).
//
// Сеть: ESP8266 сам запоминает последнюю сеть во Flash и переподключается к ней.
// Логин и пароль задаются через портал настройки (SettingsApp → WI-FI).
// Когда плата в сети, работает ArduinoOTA: pio run -e d1_mini_ota -t upload.
#pragma once

#include "core/Config.h"

#if GAMEBOX_HAS_WIFI

#include <Arduino.h>

namespace Network
{
    constexpr const char* HOSTNAME = "lcd-game-box";

    void begin();
    void update(); // вызывать каждый проход loop()

    bool connected();
    String ip();

    void connect(const char* ssid, const char* pass); // новая сеть, запоминается
} // namespace Network

#endif
