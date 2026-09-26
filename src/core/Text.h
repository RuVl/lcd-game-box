// Сборка коротких строк для экрана без snprintf: printf-семейство тянет в прошивку UNO
// около 1,3 КБ, а экрану нужны только текст и целые числа.
//
//   char buf[17];
//   char *p = Text::str(buf, "SCORE ");
//   Text::num(p, score, 6);            // "SCORE 000123"
//
// Каждая функция пишет в p, ставит '\0' и возвращает указатель на него - вызовы
// можно продолжать с этого места. За размер буфера отвечает вызывающий.
#pragma once

#include <Arduino.h>

namespace Text
{
    // Строку целиком.
    char* str(char* p, const char* s);

    // Число. width > 0 - не меньше width знаков, слева дополняется символом pad
    // ('0' - ведущие нули, ' ' - выравнивание вправо).
    char* num(char* p, uint32_t value, uint8_t width = 0, char pad = '0');

    // Один символ.
    char* chr(char* p, char c);
} // namespace Text
