#include "games/arkanoid/ArkanoidGame.h"

#include "core/Config.h"
#include "core/Display.h"
#include "core/Input.h"
#include "core/Sound.h"
#include "core/Text.h"
#include "games/arkanoid/ArkanoidData.h"

using namespace arkanoid;

namespace
{
    // Экран: счёт в колонках 0…5, поле в 6…9, уровень и жизни в 10…15.
    constexpr uint8_t CANVAS_COL = 6;
    constexpr uint8_t HUD_RIGHT_COL = 10;

    void putText(uint8_t col, uint8_t row, const char* s)
    {
        while (*s) Display::put(col++, row, *s++);
    }
} // namespace

// ---------- Кирпичи ----------

bool ArkanoidGame::brickAt(uint8_t x, uint8_t y) const
{
    return y >= BRICK_TOP && y < BRICK_TOP + BRICK_ROWS && canvas.get(x, y);
}

void ArkanoidGame::hitBrick(uint8_t x, uint8_t y)
{
    canvas.fillRect(x & ~1, y, 2, 1, false);
    brickCount--;
    score += 10 * (1 + level / LAYOUT_COUNT); // каждый новый круг раскладок - дороже
    hudDirty = true;
    Sound::play(SND_BRICK);
}

// ---------- Мяч и ракетка ----------

void ArkanoidGame::setDirection(uint8_t dir)
{
    velX = ((int8_t)pgm_read_byte(&DIR_X[dir]) * ballSpeed) >> 6;
    velY = -(((int8_t)pgm_read_byte(&DIR_Y[dir]) * ballSpeed) >> 6);
}

// Мяч лежит на ракетке над её второй точкой.
void ArkanoidGame::placeBallOnPaddle()
{
    ballX = ((paddleX + 1) << 8) + 128;
    ballY = ((PADDLE_Y - 1) << 8) + 128;
}

// Первый шаг - сразу при наклоне, дальше пауза и автоповтор: и точная доводка, и быстрый проезд.
void ArkanoidGame::movePaddle(int8_t dx)
{
    paddleDir = 0;
    if (dx != heldDir)
    {
        heldDir = dx;
        paddleWait = PADDLE_FIRST_WAIT;
    }
    else if (--paddleWait == 0)
    {
        paddleWait = PADDLE_EVERY;
    }
    else
    {
        return;
    }
    int8_t x = paddleX + dx;
    if (dx == 0 || x < 0 || x > FIELD_W - PADDLE_W) return;
    paddleX = x;
    paddleDir = dx;
    if (stuck) placeBallOnPaddle();
}

// Шаг мяча: сначала по X, потом по Y, каждое столкновение отражает свою составляющую скорости.
// За шаг мяч проходит меньше точки, поэтому достаточно проверить соседнюю клетку.
// Координаты беззнаковые: вылет за левый/верхний край даёт переполнение и ловится той же
// проверкой, что и правый/нижний.
bool ArkanoidGame::moveBall()
{
    uint8_t cx = ballX >> 8, cy = ballY >> 8;

    uint16_t nx = ballX + velX;
    uint8_t tx = nx >> 8;
    if (nx >= FIELD_W << 8)
    {
        velX = -velX;
    }
    else if (tx != cx && brickAt(tx, cy))
    {
        hitBrick(tx, cy);
        velX = -velX;
    }
    else
    {
        ballX = nx;
        cx = tx;
    }

    uint16_t ny = ballY + velY;
    uint8_t ty = ny >> 8;
    if (velY < 0 && ny >= FIELD_H << 8)
    {
        // потолок
        velY = -velY;
        return true;
    }
    if (ty != cy && brickAt(cx, ty))
    {
        hitBrick(cx, ty);
        velY = -velY;
        return true;
    }
    if (ty == PADDLE_Y && cy < PADDLE_Y)
    {
        // Куда попал мяч: −1 и PADDLE_W - угол ракетки, если мяч летит на неё сбоку.
        int8_t off = cx - paddleX;
        if ((uint8_t)off < PADDLE_W || (off == -1 && velX > 0) || (off == PADDLE_W && velX < 0))
        {
            int8_t dir = off + 1 + paddleDir; // едущая ракетка "подкручивает" мяч
            if (dir < 0) dir = 0;
            if (dir >= DIR_COUNT) dir = DIR_COUNT - 1;
            setDirection(dir);
            Sound::play(SND_PADDLE);
            return true;
        }
    }
    ballY = ny; // упавший мяч уходит за нижний край холста и просто не рисуется
    return ty < FIELD_H;
}

// ---------- Отрисовка ----------

void ArkanoidGame::render()
{
    uint8_t x = ballX >> 8, y = ballY >> 8;
    if (x != drawnX || y != drawnY) canvas.set(drawnX, drawnY, false); // стереть, только если сдвинулся
    // Каждую точку строки ракетки - сразу в нужное состояние: неподвижная ракетка не делает
    // символы "изменёнными" и не перезаливается в дисплей.
    for (uint8_t i = 0; i < FIELD_W; i++) canvas.set(i, PADDLE_Y, i >= paddleX && i < paddleX + PADDLE_W);
    canvas.set(x, y);
    drawnX = x;
    drawnY = y;
    canvas.flush();
    if (hudDirty) drawHud();
}

void ArkanoidGame::drawHud()
{
    char buf[7];
    putText(0, 0, "SCORE ");
    Text::num(buf, score, 6);
    putText(0, 1, buf);
    putText(HUD_RIGHT_COL, 0, " LV ");
    Text::num(buf, level + 1, 2);
    putText(HUD_RIGHT_COL + 4, 0, buf);
    putText(HUD_RIGHT_COL, 1, " o x");
    Display::put(HUD_RIGHT_COL + 4, 1, '0' + lives);
    hudDirty = false;
}

// ---------- Конечный автомат ----------

void ArkanoidGame::begin() { enter(State::Title); }

void ArkanoidGame::enter(State s)
{
    state = s;
    phase.reset();
}

void ArkanoidGame::update()
{
    switch (state)
    {
    case State::Title: updateTitle();
        break;
    case State::Playing: updatePlaying();
        break;
    case State::Pause: updatePause();
        break;
    case State::GameOver: updateGameOver();
        break;
    }
}

void ArkanoidGame::startLevel()
{
    uint16_t s = SPEED_BASE + level * SPEED_STEP;
    ballSpeed = s > SPEED_MAX ? SPEED_MAX : s;
    Display::clear();
    canvas.begin(CANVAS_COL, 0);
    drawnY = FIELD_H; // старого мяча на чистом холсте нет: не стирать точку, где теперь кирпич
    brickCount = 0;
    const uint16_t* layout = LAYOUTS[level % LAYOUT_COUNT];
    for (uint8_t r = 0; r < BRICK_ROWS; r++)
    {
        uint16_t bits = pgm_read_word(layout + r);
        for (uint8_t x = 0; x < FIELD_W; x += 2, bits <<= 1)
        {
            if (!(bits & (1 << (BRICK_COLS - 1)))) continue;
            canvas.fillRect(x, BRICK_TOP + r, 2, 1);
            brickCount++;
        }
    }
    paddleX = (FIELD_W - PADDLE_W) / 2;
    hudDirty = true;
    startServe();
}

// Мяч на ракетке, ждём запуска.
void ArkanoidGame::startServe()
{
    stuck = true;
    heldDir = 0;
    launchLatched = false;
    placeBallOnPaddle();
    render();
    ticker.start();
    enter(State::Playing);
}

void ArkanoidGame::updateTitle()
{
    if (phase.step == 0)
    {
        title.begin("ARKANOID", HISCORE_SLOT);
        phase.next();
    }
    switch (title.update())
    {
    case TitleScreen::START:
        lives = START_LIVES;
        score = 0;
        level = 0;
        Sound::play(SND_START);
        startLevel();
        break;
    case TitleScreen::EXIT: requestExit();
        break;
    default: break;
    }
}

void ArkanoidGame::updatePlaying()
{
    if (Input::anyPressed(Input::CLICK | Input::UP)) launchLatched = true;
    if (!ticker.due(TICK_MS)) return;

    movePaddle(Input::dx());
    if (stuck)
    {
        if (launchLatched)
        {
            stuck = false;
            // Наклон стика при запуске задаёт сторону, иначе - случайно чуть влево или вправо.
            setDirection(heldDir < 0 ? 1 : heldDir > 0 ? DIR_COUNT - 2 : 2 + (millis() & 1));
            Sound::play(SND_LAUNCH);
        }
    }
    else if (!moveBall())
    {
        lives--;
        hudDirty = true;
        Sound::play(lives ? SND_LOST : SND_GAMEOVER);
        enter(State::Pause);
    }
    launchLatched = false;
    render();

    if (state == State::Playing && brickCount == 0)
    {
        score += 50 * (level + 1);
        drawHud();
        putText(HUD_RIGHT_COL, 0, "CLEAR!");
        Sound::play(SND_CLEAR);
        enter(State::Pause);
    }
}

// Пауза после потерянного мяча или пройденного уровня: ждём конца мелодии.
void ArkanoidGame::updatePause()
{
    if (Sound::playing() || phase.elapsed() < 800) return;
    if (brickCount == 0)
    {
        if (level < 98) level++;
        startLevel();
    }
    else if (lives)
    {
        startServe();
    }
    else
    {
        enter(State::GameOver);
    }
}

// Итог и рекорд; на титульный экран - по таймеру или нажатию.
void ArkanoidGame::updateGameOver()
{
    if (phase.step == 0)
    {
        result.begin("GAME OVER", score, HISCORE_SLOT);
        phase.next();
    }
    else if (result.update())
    {
        enter(State::Title);
    }
}
