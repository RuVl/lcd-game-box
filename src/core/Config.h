// Подключение железа и общие настройки устройства. Схемы подключения - в README.
#pragma once

#include <Arduino.h>

// Версия прошивки. Для обновлений по Wi-Fi она должна совпадать с version в project.json.
#define GAMEBOX_VERSION "0.2.0"

// Возможности платы: всегда 0 или 1, чтобы "#if GAMEBOX_..." без подключённого Config.h не
// превращался молча в "нет" (файлы, которые их проверяют, делают #error, если макрос не определён).
#if defined(ESP8266)
#define GAMEBOX_HAS_WIFI 1 // Wi-Fi и обновления по воздуху
#define GAMEBOX_SMALL 0 // полный список игр
#else
#define GAMEBOX_HAS_WIFI 0
#define GAMEBOX_SMALL 1 // 32 КБ Flash: только игры, помеченные для UNO (см. games/Registry.cpp)
#endif

namespace config
{
#if defined(ESP8266)
    // ---------- Wemos D1 mini (ESP8266) ----------
    // LCD1602 через сдвиговый регистр 74HC595: Q1→RS, Q3→E, Q4…Q7→D4…D7.
    constexpr uint8_t PIN_SR_DATA = D7; // 74HC595 DS (14)
    constexpr uint8_t PIN_SR_CLOCK = D5; // 74HC595 SH_CP (11)
    constexpr uint8_t PIN_SR_LATCH = D6; // 74HC595 ST_CP (12)
    constexpr uint8_t SR_Q_RS = 1, SR_Q_E = 3, SR_Q_D4 = 4, SR_Q_D5 = 5, SR_Q_D6 = 6, SR_Q_D7 = 7;

    // Джойстик: у ESP8266 один аналоговый вход, поэтому оси подключаются к A0 по очереди
    // через диоды. Вывод MUX в LOW «отключает" свою ось, в режиме входа - подключает.
    constexpr uint8_t PIN_JOY_ADC = A0;
    constexpr uint8_t PIN_MUX_X = D1;
    constexpr uint8_t PIN_MUX_Y = D2;
    constexpr uint8_t PIN_JOY_SW = D3; // GPIO0: не держать стик нажатым при включении
    constexpr uint8_t PIN_BUZZER = D8;

    constexpr uint16_t JOY_SAMPLE_MS = 10; // частый analogRead на ESP8266 мешает Wi-Fi
#else
    // ---------- Arduino UNO ----------
    constexpr uint8_t PIN_LCD_RS = 12;
    constexpr uint8_t PIN_LCD_E = 11;
    constexpr uint8_t PIN_LCD_D4 = 2;
    constexpr uint8_t PIN_LCD_D5 = 3;
    constexpr uint8_t PIN_LCD_D6 = 4;
    constexpr uint8_t PIN_LCD_D7 = 5;

    constexpr uint8_t PIN_JOY_X = A0;
    constexpr uint8_t PIN_JOY_Y = A1;
    constexpr uint8_t PIN_JOY_SW = 8;
    constexpr uint8_t PIN_BUZZER = 9;

    constexpr uint16_t JOY_SAMPLE_MS = 5;
#endif

    // Если джойстик повёрнут, поменяйте эти флаги.
    constexpr bool JOY_INVERT_X = false;
    constexpr bool JOY_INVERT_Y = false; // false: «вверх" даёт значение меньше центра

    // Калибровка, чувствительность и дребезг настраиваются в меню JOYSTICK SETUP и хранятся в EEPROM.
    // Наклон считается от центра в процентах хода в эту сторону. Гистерезис: направление включается
    // дальше от центра, чем выключается, - стик у порога не дребезжит.
    constexpr uint8_t DEFAULT_SENSITIVITY = 9; // порог - 38 % хода стика (1 - 95 %, 10 - 30 %)
    constexpr uint8_t DEFAULT_DEBOUNCE = 3; // × 10 мс
    constexpr uint16_t REPEAT_DELAY_MS = 400; // удержание: через сколько начинается автоповтор
    constexpr uint16_t REPEAT_STEP_MS = 120; // и с каким шагом

    // Сколько держать стик нажатым, чтобы выйти из игры в меню (там, где игра это разрешает).
    constexpr uint16_t HOLD_TO_EXIT_MS = 1000;
} // namespace config
