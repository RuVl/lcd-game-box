#include "core/Display.h"

#include "core/Config.h"

namespace Display
{
#if defined(ESP8266)
    Lcd lcd(config::PIN_SR_DATA, config::PIN_SR_CLOCK, config::PIN_SR_LATCH, config::SR_Q_RS, config::SR_Q_E,
            config::SR_Q_D4, config::SR_Q_D5, config::SR_Q_D6, config::SR_Q_D7);
#else
    Lcd lcd(config::PIN_LCD_RS, config::PIN_LCD_E, config::PIN_LCD_D4, config::PIN_LCD_D5, config::PIN_LCD_D6,
            config::PIN_LCD_D7);
#endif

    namespace
    {
        // Что сейчас на экране. 0 - "неизвестно", поэтому символ CGRAM 0 хранится как 8
        // (у HD44780 коды 0 и 8 - один и тот же пользовательский символ).
        uint8_t shadow[ROWS][COLS];

        void forgetRow(uint8_t row) { memset(shadow[row], 0, COLS); }
    } // namespace

    void begin()
    {
        lcd.begin(COLS, ROWS);
        clear();
    }

    void clear()
    {
        lcd.clear();
        memset(shadow, 0, sizeof(shadow));
    }

    void put(uint8_t col, uint8_t row, uint8_t c)
    {
        if (c == 0) c = 8;
        if (shadow[row][col] == c) return;
        shadow[row][col] = c;
        lcd.setCursor(col, row);
        lcd.write(c);
    }

    void printRow(uint8_t row, const char* text)
    {
        lcd.setCursor(0, row);
        lcd.print(text);
        forgetRow(row);
    }

    void printCentered(uint8_t row, const char* text)
    {
        uint8_t len = strlen(text);
        lcd.setCursor(0, row);
        lcd.print(F("                "));
        lcd.setCursor(len < COLS ? (COLS - len) / 2 : 0, row);
        lcd.print(text);
        forgetRow(row);
    }

    void loadGlyph(uint8_t slot, const uint8_t* progmemRows, bool mirror)
    {
        uint8_t buf[8];
        for (uint8_t i = 0; i < 8; i++)
        {
            uint8_t row = pgm_read_byte(progmemRows + i);
            if (mirror)
            {
                uint8_t m = 0;
                for (uint8_t b = 0; b < 5; b++)
                    if (row & (1 << b)) m |= 1 << (4 - b);
                row = m;
            }
            buf[i] = row;
        }
        lcd.createChar(slot, buf);
    }
} // namespace Display
