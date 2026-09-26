#include "games/tetris/TetrisGame.h"

#include "core/Config.h"
#include "core/Display.h"
#include "core/Input.h"
#include "core/Sound.h"
#include "core/Text.h"
#include "games/tetris/TetrisData.h"

using namespace tetris;

namespace
{
    constexpr uint8_t HISCORE_SLOT = 1;
    constexpr uint16_t TICK_MS = 16; // шаг игры ≈ кадр NES
    constexpr uint8_t SOFT_DROP_TICKS = 3;
    constexpr uint16_t FULL_ROW = 0x1FF8; // клетки x = 0…9 - биты 12…3
    constexpr uint16_t WALLS = 0xE007;
    constexpr uint32_t MAX_SCORE = 999999;

    // Раскладка экрана
    constexpr uint8_t FIELD_COL = 6; // поле 2×2 знакоместа: колонки 6…7
    constexpr uint8_t SLOT_WALL_L = 4, SLOT_WALL_R = 5, SLOT_NEXT = 6;
    constexpr uint8_t NEXT_COL = 1; // следующая фигура: колонки 1…2 нижней строки
    constexpr uint8_t STATS_COL = 10;

    // Сдвиги при повороте у стены: на месте, затем на 1 и на 2 клетки (2 - для палки).
    constexpr int8_t KICKS[] PROGMEM = {0, -1, 1, -2, 2};

    // Строка r квадрата фигуры, сдвинутая в столбец x (в битах rows; x от −3 до 9).
    uint16_t shapeRow(uint16_t s, uint8_t r, int8_t x) { return ((s >> (12 - 4 * r)) & 0xF) << (9 - x); }

    void putText(uint8_t col, uint8_t row, const char* s)
    {
        while (*s && col < Display::COLS) Display::put(col++, row, *s++);
    }
} // namespace

// ---------- Фигура и стакан ----------

uint16_t TetrisGame::shape(uint8_t rotation) const { return pgm_read_word(&SHAPES[piece][rotation]); }

// Помещается ли фигура в повороте rotation с углом в (x, y). Выше стакана места сколько угодно.
bool TetrisGame::fits(uint8_t rotation, int8_t x, int8_t y) const
{
    if (x < -3 || x > 9) return false; // дальше стены 16 бит строки не дотягиваются
    uint16_t s = shape(rotation);
    for (uint8_t r = 0; r < 4; r++)
    {
        uint16_t m = shapeRow(s, r, x);
        if (!m) continue;
        int8_t fy = y + r;
        if (fy >= H || (m & WALLS) || (fy >= 0 && (rows[fy] & m))) return false;
    }
    return true;
}

// Клетки текущей фигуры в строке стакана y.
uint16_t TetrisGame::pieceRow(int8_t y) const
{
    uint8_t r = y - py;
    return r < 4 ? shapeRow(shape(rot), r, px) : 0;
}

// "Мешок": все 7 фигур в случайном порядке, потом новый мешок - долгих засух без палки не бывает.
uint8_t TetrisGame::takeFromBag()
{
    if (bag == (1 << PIECE_COUNT) - 1) bag = 0;
    uint8_t i;
    do i = random(PIECE_COUNT);
    while (bag & (1 << i));
    bag |= 1 << i;
    return i;
}

// Новая фигура сверху по центру. false - ей нет места, игра окончена.
bool TetrisGame::spawn()
{
    piece = next;
    next = takeFromBag();
    rot = 0;
    px = 3;
    py = piece == P_I ? -1 : 0; // у палки занята вторая строка квадрата
    fallTicks = 0;
    softDropBlocked = Input::held(Input::DOWN);
    drawNext();
    return fits(rot, px, py);
}

bool TetrisGame::tryMove(int8_t dx, int8_t dy)
{
    if (!fits(rot, px + dx, py + dy)) return false;
    px += dx;
    py += dy;
    return true;
}

void TetrisGame::rotate()
{
    uint8_t r = (rot + 1) & 3;
    for (uint8_t i = 0; i < sizeof(KICKS); i++)
    {
        int8_t dx = pgm_read_byte(&KICKS[i]);
        if (fits(r, px + dx, py))
        {
            rot = r;
            px += dx;
            Sound::play(SND_ROTATE);
            return;
        }
    }
}

// Фигура легла: вписать в стакан, проверить линии и конец игры.
void TetrisGame::lock()
{
    bool toppedOut = false;
    for (int8_t y = py; y < py + 4; y++)
    {
        uint16_t bits = pieceRow(y);
        if (!bits) continue;
        if (y < 0)
            toppedOut = true; // часть фигуры осталась над стаканом
        else
            rows[y] |= bits;
    }
    fullRows = 0;
    uint8_t n = 0;
    for (uint8_t y = 0; y < H; y++)
    {
        if (rows[y] == FULL_ROW)
        {
            fullRows |= 1 << y;
            n++;
        }
    }
    if (n)
    {
        Sound::play(n == 4 ? SND_TETRIS : SND_LINE);
        addScore((uint32_t)pgm_read_word(&LINE_SCORE[n - 1]) * (level + 1));
        lines += n;
        render(false);
        enter(State::Clearing);
        return;
    }
    Sound::play(SND_LOCK);
    if (toppedOut || !spawn())
    {
        enter(State::Topping);
        return;
    }
    render();
}

void TetrisGame::addScore(uint32_t points)
{
    score = score + points > MAX_SCORE ? MAX_SCORE : score + points;
}

void TetrisGame::removeFullRows()
{
    int8_t dst = H - 1;
    for (int8_t y = H - 1; y >= 0; y--)
        if (!(fullRows & (1 << y))) rows[dst--] = rows[y];
    while (dst >= 0) rows[dst--] = 0;
    fullRows = 0;
}

// ---------- Отрисовка ----------

// Поле - 4 символа CGRAM: слева вверху 0, справа 1, внизу 2 и 3. Строка символа - 5 клеток строки
// стакана. Загружаются только изменившиеся символы. Мигающие линии гаснут на нечётных шагах Clearing.
void TetrisGame::render(bool withPiece)
{
    bool hideFull = state == State::Clearing && (phase.step & 1);
    for (uint8_t cell = 0; cell < 4; cell++)
    {
        uint8_t g[8];
        for (uint8_t i = 0; i < 8; i++)
        {
            uint8_t y = (cell & 2) * 4 + i;
            uint16_t bits = rows[y];
            if (hideFull && (fullRows & (1 << y))) bits = 0;
            if (withPiece) bits |= pieceRow(y);
            g[i] = (bits >> ((cell & 1) ? 3 : 8)) & 0x1F;
        }
        if (memcmp(g, shown[cell], 8))
        {
            memcpy(shown[cell], g, 8);
            Display::lcd.createChar(cell, g);
        }
    }
    drawStats();
}

// Следующая фигура в двух символах CGRAM (10×8 точек), клетка - квадрат 2×2 точки.
void TetrisGame::drawNext()
{
    uint16_t s = pgm_read_word(&SHAPES[next][0]);
    uint8_t half = next == P_I || next == P_O; // широкую и узкую фигуры сдвинуть на полклетки влево
    uint8_t g[2][8] = {};
    for (uint8_t i = 2; i < 6; i++)
    {
        // строки 2…5: две строки фигуры по 2 точки
        uint8_t nib = (s >> (12 - 4 * ((i - 2) >> 1))) & 0xF;
        uint16_t dots = 0;
        for (uint8_t c = 0; c < 4; c++)
            if (nib & (8 >> c)) dots |= 3 << (6 - 2 * c + half);
        g[0][i] = dots >> 5;
        g[1][i] = dots & 0x1F;
    }
    Display::lcd.createChar(SLOT_NEXT, g[0]);
    Display::lcd.createChar(SLOT_NEXT + 1, g[1]);
}

void TetrisGame::drawStats()
{
    char buf[8];
    char* p = Text::num(Text::str(buf, "LV"), level + 1);
    while (p < buf + 4) p = Text::chr(p, ' '); // "LV1 " - затереть вторую цифру прошлого уровня
    putText(0, 0, buf);
    Text::num(buf, score, 6);
    putText(STATS_COL, 0, buf);
    p = Text::num(Text::str(buf, "L "), lines);
    while (p < buf + 6) p = Text::chr(p, ' ');
    putText(STATS_COL, 1, buf);
}

// ---------- Конечный автомат ----------

void TetrisGame::begin() { enter(State::Title); }

void TetrisGame::enter(State s)
{
    state = s;
    phase.reset();
}

void TetrisGame::update()
{
    switch (state)
    {
    case State::Title: updateTitle();
        break;
    case State::Playing: updatePlaying();
        break;
    case State::Clearing: updateClearing();
        break;
    case State::Topping: updateTopping();
        break;
    case State::Result: updateResult();
        break;
    }
}

void TetrisGame::updateTitle()
{
    if (phase.step == 0)
    {
        title.begin("TETRIS", HISCORE_SLOT);
        phase.next();
    }
    switch (title.update())
    {
    case TitleScreen::START: startGame();
        break;
    case TitleScreen::EXIT: requestExit();
        break;
    default: break;
    }
}

void TetrisGame::startGame()
{
    memset(rows, 0, sizeof(rows));
    score = 0;
    lines = 0;
    level = 0;
    bag = 0;
    pending = 0;
    next = takeFromBag();

    Display::clear();
    memset(shown, 0xFF, sizeof(shown)); // загрузить все символы поля заново
    Display::loadGlyph(SLOT_WALL_L, GLYPH_WALL_L);
    Display::loadGlyph(SLOT_WALL_R, GLYPH_WALL_R);
    for (uint8_t r = 0; r < 2; r++)
    {
        Display::put(FIELD_COL - 1, r, SLOT_WALL_L);
        Display::put(FIELD_COL, r, 2 * r);
        Display::put(FIELD_COL + 1, r, 2 * r + 1);
        Display::put(FIELD_COL + 2, r, SLOT_WALL_R);
    }
    Display::put(NEXT_COL, 1, SLOT_NEXT);
    Display::put(NEXT_COL + 1, 1, SLOT_NEXT + 1);
    spawn();
    Sound::play(SND_START);
    enter(State::Playing);
    render();
    ticker.start();
}

void TetrisGame::updatePlaying()
{
    // Нажатия копятся между шагами, чтобы ни одно не потерялось.
    if (Input::repeat(Input::LEFT)) pending |= Input::LEFT;
    if (Input::repeat(Input::RIGHT)) pending |= Input::RIGHT;
    // Поворот - и нажатием, и ↑: мгновенного сброса нет, случайный наклон вверх не роняет фигуру.
    if (Input::pressed(Input::UP | Input::CLICK)) pending |= Input::CLICK;
    if (!ticker.due(TICK_MS)) return;
    gameTick();
}

void TetrisGame::gameTick()
{
    uint8_t in = pending;
    pending = 0;
    bool moved = false;
    if (in & Input::CLICK)
    {
        rotate();
        moved = true;
    }
    bool shifted = ((in & Input::LEFT) && tryMove(-1, 0)) || ((in & Input::RIGHT) && tryMove(1, 0));
    if (shifted && !Sound::playing()) Sound::play(SND_MOVE);
    moved |= shifted;

    if (!Input::held(Input::DOWN)) softDropBlocked = false;
    bool soft = !softDropBlocked && Input::held(Input::DOWN);
    uint8_t g = pgm_read_byte(&GRAVITY[level < GRAVITY_LEVELS ? level : GRAVITY_LEVELS - 1]);
    if (soft && g > SOFT_DROP_TICKS) g = SOFT_DROP_TICKS;
    if (++fallTicks >= g)
    {
        fallTicks = 0;
        if (!tryMove(0, 1))
        {
            lock();
            return;
        }
        if (soft) addScore(1); // ускоренное падение: 1 очко за строку
        moved = true;
    }
    if (moved) render();
}

// Заполненные линии мигают три раза, потом исчезают.
void TetrisGame::updateClearing()
{
    if (phase.elapsed() < 80) return;
    phase.next();
    if (phase.step < 6)
    {
        render(false);
        return;
    }
    removeFullRows();
    uint8_t oldLevel = level;
    level = lines / 10;
    if (level != oldLevel) Sound::play(SND_LEVEL);
    pending = 0;
    if (!spawn())
    {
        enter(State::Topping);
        return;
    }
    enter(State::Playing);
    render();
    ticker.start();
}

// Конец игры: стакан заливается снизу вверх.
void TetrisGame::updateTopping()
{
    if (phase.step == 0)
    {
        Sound::play(SND_GAME_OVER);
        render(false);
        phase.next();
    }
    else if (phase.step <= H)
    {
        if (phase.elapsed() < 45) return;
        rows[H - phase.step] = FULL_ROW;
        render(false);
        phase.next();
    }
    else if (!Sound::playing() && phase.elapsed() >= 400)
    {
        enter(State::Result);
    }
}

// Итог: очки и линии, рекорд; на титульный экран - по таймеру или нажатию.
void TetrisGame::updateResult()
{
    if (phase.step == 0)
    {
        char buf[17];
        Text::num(Text::str(Text::num(buf, score, 6), "  L"), lines);
        result.begin("GAME OVER", score, HISCORE_SLOT, buf);
        phase.next();
    }
    else if (result.update())
    {
        enter(State::Title);
    }
}
