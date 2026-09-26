#include "core/PixelCanvas.h"

#include "core/Display.h"

void PixelCanvas::begin(uint8_t col, uint8_t row, uint8_t cols, uint8_t rows, uint8_t firstSlot)
{
    col_ = col;
    row_ = row;
    cols_ = cols;
    rows_ = rows;
    firstSlot_ = firstSlot;
    clear();
    invalidate();
    redrawCells();
}

void PixelCanvas::clear()
{
    for (uint8_t i = 0; i < cols_ * rows_; i++)
    {
        for (uint8_t y = 0; y < CELL_H; y++)
        {
            if (cells_[i][y])
            {
                cells_[i][y] = 0;
                dirty_ |= 1 << i;
            }
        }
    }
}

void PixelCanvas::set(uint8_t x, uint8_t y, bool on)
{
    if (x >= width() || y >= height()) return;
    uint8_t i = (y / CELL_H) * cols_ + x / CELL_W;
    uint8_t bit = 1 << (CELL_W - 1 - x % CELL_W);
    uint8_t& line = cells_[i][y % CELL_H];
    uint8_t next = on ? line | bit : line & ~bit;
    if (next != line)
    {
        line = next;
        dirty_ |= 1 << i;
    }
}

bool PixelCanvas::get(uint8_t x, uint8_t y) const
{
    if (x >= width() || y >= height()) return false;
    uint8_t i = (y / CELL_H) * cols_ + x / CELL_W;
    return cells_[i][y % CELL_H] & (1 << (CELL_W - 1 - x % CELL_W));
}

void PixelCanvas::fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool on)
{
    for (uint8_t dy = 0; dy < h; dy++)
        for (uint8_t dx = 0; dx < w; dx++) set(x + dx, y + dy, on);
}

void PixelCanvas::drawBitmap(uint8_t x, uint8_t y, const uint8_t* progmemRows, uint8_t w, uint8_t h, bool on)
{
    for (uint8_t dy = 0; dy < h; dy++)
    {
        uint8_t bits = pgm_read_byte(progmemRows + dy);
        for (uint8_t dx = 0; dx < w; dx++)
            if (bits & (0x80 >> dx)) set(x + dx, y + dy, on);
    }
}

void PixelCanvas::flush()
{
    if (!dirty_) return;
    for (uint8_t i = 0; i < cols_ * rows_; i++)
    {
        if (!(dirty_ & (1 << i))) continue;
        Display::lcd.createChar(firstSlot_ + i, cells_[i]);
    }
    dirty_ = 0;
}

void PixelCanvas::invalidate() { dirty_ = 0xFF; }

void PixelCanvas::redrawCells()
{
    for (uint8_t r = 0; r < rows_; r++)
        for (uint8_t c = 0; c < cols_; c++) Display::put(col_ + c, row_ + r, firstSlot_ + r * cols_ + c);
}
