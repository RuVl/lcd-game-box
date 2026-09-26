#include "core/Text.h"

namespace Text
{
    char* str(char* p, const char* s)
    {
        while (*s) *p++ = *s++;
        *p = '\0';
        return p;
    }

    char* num(char* p, uint32_t value, uint8_t width, char pad)
    {
        char digits[10]; // uint32_t - не больше 10 цифр
        uint8_t n = 0;
        do
        {
            digits[n++] = '0' + value % 10;
            value /= 10;
        }
        while (value);
        while (width > n)
        {
            *p++ = pad;
            width--;
        }
        while (n) *p++ = digits[--n];
        *p = '\0';
        return p;
    }

    char* chr(char* p, char c)
    {
        *p++ = c;
        *p = '\0';
        return p;
    }
} // namespace Text
