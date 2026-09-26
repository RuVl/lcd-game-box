// LCD1602: общий экземпляр дисплея и вывод с буфером изменений.
#pragma once

#include <Arduino.h>

#if defined(ESP8266)
#include <LiquidCrystal_74HC595.h>
using Lcd = LiquidCrystal_74HC595; // LCD через сдвиговый регистр
#else
#include <LiquidCrystal.h>
using Lcd = LiquidCrystal; // LCD напрямую, 4-битный режим
#endif

namespace Display
{
    constexpr uint8_t COLS = 16;
    constexpr uint8_t ROWS = 2;

    extern Lcd lcd;

    void begin();

    // Очистить экран и забыть, что на нём было.
    void clear();

    // Вывести символ, только если в этой клетке сейчас другой. Символ 0 (CGRAM 0) допустим.
    // Удобно для игрового поля: перерисовка всего экрана каждый кадр стоит только изменений.
    void put(uint8_t col, uint8_t row, uint8_t c);

    // Вывести строку целиком (ровно COLS символов).
    void printRow(uint8_t row, const char* text);

    // Строка по центру (остаток строки затирается пробелами).
    void printCentered(uint8_t row, const char* text);

    // Загрузить пользовательский символ 5×8 из PROGMEM, при необходимости отразив по горизонтали.
    void loadGlyph(uint8_t slot, const uint8_t* progmemRows, bool mirror = false);
} // namespace Display
