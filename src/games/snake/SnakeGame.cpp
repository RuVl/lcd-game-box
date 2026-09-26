#include "games/snake/SnakeGame.h"

#include "core/Config.h"
#include "core/Display.h"
#include "core/Input.h"
#include "core/Sound.h"
#include "core/Storage.h"
#include "games/snake/SnakeData.h"

using namespace snake;

namespace
{
    constexpr uint8_t HISCORE_SLOT = 2; // слот рекорда в EEPROM
    constexpr uint8_t START_LEN = 3;
    constexpr uint16_t START_DELAY_MS = 600;

    // Скорость: старт 250 мс на шаг, каждая еда −4 мс, не быстрее 110 мс (≈35 еды).
    constexpr uint16_t TICK_START_MS = 250;
    constexpr uint8_t TICK_STEP_MS = 4;
    constexpr uint16_t TICK_MIN_MS = 110;

    // Шаг клетки в направлении d (0 →, 1 ↓, 2 ←, 3 ↑) с переходом через край.
    // false - клетка пересекла край поля (в режиме стен это смерть).
    bool moveCell(uint8_t& x, uint8_t& y, uint8_t d, uint8_t w, uint8_t h)
    {
        x += (d == 0) - (d == 2);
        y += (d == 1) - (d == 3);
        bool inside = x < w && y < h;
        if (x == 0xFF) x = w - 1;
        if (x == w) x = 0;
        if (y == 0xFF) y = h - 1;
        if (y == h) y = 0;
        return inside;
    }

    // Текст из Flash прямо в LCD, мимо буфера Display::put (для надписей, которые put() не трогает;
    // итог игры перекрывает цифры счёта, но после него всё равно Display::clear()).
    void printAt(uint8_t col, uint8_t row, const __FlashStringHelper* text)
    {
        Display::lcd.setCursor(col, row);
        Display::lcd.print(text);
    }

    // Число из 4 цифр в колонках 12…15.
    void putNumber(uint8_t row, uint16_t v)
    {
        for (uint8_t col = 15; col >= 12; col--)
        {
            Display::put(col, row, '0' + v % 10);
            v /= 10;
        }
    }
} // namespace

// ---------- Тело змейки ----------

uint8_t SnakeGame::getDir(uint16_t i) const { return (dirs[i / 4] >> (i % 4 * 2)) & 3; }

void SnakeGame::setDir(uint16_t i, uint8_t d)
{
    uint8_t shift = i % 4 * 2;
    dirs[i / 4] = (dirs[i / 4] & ~(3 << shift)) | (d << shift);
}

// Еда - в случайную свободную клетку. На холсте в этот момент только змейка.
void SnakeGame::spawnFood()
{
    uint16_t r = CELLS - 1 - steps; // свободных клеток
    hasFood = r > 0;
    if (!hasFood) return;
    r = random(r);
    for (foodY = 0; foodY < H; foodY++)
        for (foodX = 0; foodX < W; foodX++)
            if (!canvas.get(foodX, foodY) && r-- == 0) return;
}

// Поворот в очередь: относительно последнего запомненного направления, без разворота назад.
void SnakeGame::queueTurn(uint8_t d)
{
    uint8_t last = turnCount ? turns[turnCount - 1] : dir;
    if ((d & 1) == (last & 1) || turnCount == 2) return;
    turns[turnCount++] = d;
}

void SnakeGame::renderStatus()
{
    putNumber(0, score);
    putNumber(1, steps + 1);
}

// Один шаг: голова вперёд, хвост подтягивается (если не съели еду).
bool SnakeGame::step()
{
    if (turnCount)
    {
        dir = turns[0];
        turns[0] = turns[1];
        turnCount--;
    }
    uint8_t nx = headX, ny = headY;
    if (!moveCell(nx, ny, dir, W, H) && !wrap) return false;
    bool eat = hasFood && nx == foodX && ny == foodY;
    // Клетка хвоста освободится в этом же шаге - туда можно
    if (!eat && canvas.get(nx, ny) && !(nx == tailX && ny == tailY)) return false;

    uint16_t headIdx = tailIdx + steps;
    setDir(headIdx < CELLS ? headIdx : headIdx - CELLS, dir);
    if (eat)
    {
        steps++;
        score += 1 + eaten / 8; // чем быстрее змейка, тем дороже еда
        eaten++;
        Sound::play(SND_EAT);
    }
    else
    {
        canvas.set(tailX, tailY, false);
        moveCell(tailX, tailY, getDir(tailIdx), W, H);
        if (++tailIdx == CELLS) tailIdx = 0;
    }
    headX = nx;
    headY = ny;
    canvas.set(headX, headY);
    if (eat) spawnFood();
    return true;
}

// ---------- Конечный автомат ----------

void SnakeGame::begin()
{
    canvas.begin(0, 0); // поле 20×16 в колонках 0…3
    enter(State::Title);
}

void SnakeGame::enter(State s)
{
    state = s;
    phase.reset();
}

void SnakeGame::update()
{
    switch (state)
    {
    case State::Title: updateTitle();
        break;
    case State::Playing: updatePlaying();
        break;
    case State::Dying: updateDying();
        break;
    case State::Final: updateFinal();
        break;
    }
}

void SnakeGame::startPlaying()
{
    Display::clear();
    canvas.clear();
    canvas.invalidate();
    canvas.redrawCells();
    // Край поля: сплошной - стена, пунктир - проход насквозь
    char edge = wrap ? ':' : '|';
    Display::put(4, 0, edge);
    Display::put(4, 1, edge);
    printAt(6, 0, F("SCORE"));
    printAt(6, 1, F("LENGTH"));

    // Змейка из 3 точек посередине, ползёт вправо (все шаги в буфере - 0, вправо)
    memset(dirs, 0, sizeof(dirs));
    tailIdx = 0;
    steps = START_LEN - 1;
    tailX = W / 2 - START_LEN;
    tailY = headY = H / 2;
    headX = tailX + steps;
    dir = 0;
    turnCount = 0;
    eaten = 0;
    score = 0;
    won = false;
    for (uint8_t x = tailX; x <= headX; x++) canvas.set(x, headY);
    spawnFood();
    renderStatus();
    ticker.start(START_DELAY_MS);
    enter(State::Playing);
}

const char* SnakeGame::modeLine() const { return wrap ? "\x7F WRAP \x7E" : "\x7F WALLS \x7E"; }

// Нажатие - старт, ←/→ - режим, удержание - выход в меню устройства.
void SnakeGame::updateTitle()
{
    if (phase.step == 0)
    {
        title.begin("SNAKE", HISCORE_SLOT, modeLine());
        phase.next();
    }
    if (Input::anyPressed(Input::LEFT | Input::RIGHT))
    {
        wrap = !wrap;
        Sound::play(SND_MODE);
        title.setExtra(modeLine());
    }
    switch (title.update())
    {
    case TitleScreen::START:
        Sound::play(SND_START);
        startPlaying();
        break;
    case TitleScreen::EXIT: requestExit();
        break;
    default: break;
    }
}

void SnakeGame::updatePlaying()
{
    // Повороты ловим на каждом проходе, применяем по одному за шаг. Берётся только главное
    // направление стика: наклон влево с лёгким уводом вверх - один поворот, а не два.
    uint8_t in = Input::direction();
    if (in && in != lastInput)
    {
        // 0 →, 1 ↓, 2 ←, 3 ↑
        queueTurn(in == Input::RIGHT ? 0 : in == Input::DOWN ? 1 : in == Input::LEFT ? 2 : 3);
    }
    lastInput = in;

    if (hasFood) canvas.set(foodX, foodY, millis() & 256); // еда мигает: 256 мс горит, 256 - нет

    uint16_t faster = eaten * TICK_STEP_MS;
    uint16_t tick = faster < TICK_START_MS - TICK_MIN_MS ? TICK_START_MS - faster : TICK_MIN_MS;
    if (Input::held(Input::CLICK)) tick /= 2; // зажатая кнопка - ускорение
    if (ticker.due(tick))
    {
        if (!step())
        {
            Sound::play(SND_OVER);
            enter(State::Dying);
        }
        else
        {
            renderStatus();
            if (!hasFood)
            {
                // змейка заняла всё поле
                won = true;
                Sound::play(SND_WIN);
                enter(State::Dying);
            }
        }
    }
    canvas.flush();
}

// Голова мигает там, где змейка разбилась, потом итог.
void SnakeGame::updateDying()
{
    if (phase.elapsed() < 150) return;
    phase.next();
    canvas.set(headX, headY, !(phase.step & 1));
    canvas.flush();
    if (phase.step == 8) enter(State::Final);
}

// Итог - справа от поля, чтобы было видно, где змейка разбилась. Рекорд, затем титульный экран
// (по таймеру или нажатию).
void SnakeGame::updateFinal()
{
    if (phase.step == 0)
    {
        printAt(5, 0, won ? F(" YOU WIN!  ") : F(" GAME OVER "));
        printAt(6, 1, F("SCORE "));
        putNumber(1, score);
        phase.next();
    }
    else if (phase.step == 1)
    {
        if (phase.elapsed() < 1500) return;
        if (score > Storage::loadHiScore(HISCORE_SLOT))
        {
            Storage::saveHiScore(HISCORE_SLOT, score);
            Sound::play(SND_START);
            printAt(5, 0, F("NEW RECORD!"));
        }
        phase.next();
    }
    else if (phase.elapsed() >= 3500 || Input::anyPressed(Input::CLICK))
    {
        enter(State::Title);
    }
}
