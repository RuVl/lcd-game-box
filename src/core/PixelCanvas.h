// Пиксельный режим: несколько знакомест LCD, склеенных в холст из отдельных точек.
//
// У HD44780 8 пользовательских символов по 5×8 точек. Холст раскладывает их прямоугольником
// cols × rows знакомест (cols × rows ≤ 8) и даёт рисовать по точкам:
//
//   4×2 знакоместа → 20×16 точек (по умолчанию)      8×1 → 40×8      2×2 → 10×16
//
// Между знакоместами на стекле есть зазор в 1 точку - это видно, но для игр не мешает.
// Остальные знакоместа экрана остаются обычным текстом (счёт, подсказки).
//
//   PixelCanvas canvas;                 // поле класса игры: буфер 64 байта живёт в её памяти
//   canvas.begin(6, 0);                 // холст 4×2 в колонках 6…9, символы CGRAM 0…7
//   canvas.clear();
//   canvas.set(x, y);                   // x: 0…width()-1 слева направо, y: 0…height()-1 сверху вниз
//   canvas.flush();                     // загрузить в дисплей изменившиеся символы
//
// flush() перезаписывает только изменённые символы (≈0,4 мс на символ на UNO), поэтому его
// можно вызывать каждый кадр. Символы CGRAM с firstSlot по firstSlot + cols×rows − 1
// принадлежат холсту; свободные остаются игре для своих значков.
#pragma once

#include <Arduino.h>

class PixelCanvas
{
public:
    static constexpr uint8_t CELL_W = 5;
    static constexpr uint8_t CELL_H = 8;
    static constexpr uint8_t MAX_CELLS = 8;

    // Разместить холст на экране: левый верхний угол (col, row), размер в знакоместах.
    // Сразу выводит знакоместа холста на экран (они показывают символы CGRAM firstSlot…).
    void begin(uint8_t col, uint8_t row, uint8_t cols = 4, uint8_t rows = 2, uint8_t firstSlot = 0);

    uint8_t width() const { return cols_ * CELL_W; }
    uint8_t height() const { return rows_ * CELL_H; }

    void clear();
    void set(uint8_t x, uint8_t y, bool on = true); // точки за пределами холста игнорируются
    bool get(uint8_t x, uint8_t y) const; // за пределами - false
    void fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool on = true);

    // Нарисовать картинку из PROGMEM: h строк, в каждой строке w ≤ 8 точек, старший бит - левая.
    void drawBitmap(uint8_t x, uint8_t y, const uint8_t* progmemRows, uint8_t w, uint8_t h, bool on = true);

    void flush(); // загрузить изменённые символы в дисплей
    void invalidate(); // считать все символы изменёнными (например, после Display::clear())
    void redrawCells(); // заново вывести знакоместа холста на экран (после Display::clear())

private:
    uint8_t cells_[MAX_CELLS][CELL_H]; // строки символов: бит 4 - левая точка, бит 0 - правая
    uint8_t dirty_ = 0; // какие символы изменились с последнего flush()
    uint8_t col_ = 0, row_ = 0, cols_ = 4, rows_ = 2, firstSlot_ = 0;
};
