#include "games/flappy/FlappyGame.h"

#include "core/Config.h"
#include "core/Display.h"
#include "core/Input.h"
#include "core/Sound.h"
#include "core/Text.h"
#include "games/flappy/FlappyData.h"

using namespace flappy;

namespace
{
    // Текст справа от холста: с колонки TEXT_COL до конца строки, остаток затирается.
    void putText(uint8_t row, const char* s)
    {
        for (uint8_t col = TEXT_COL; col < Display::COLS; col++) Display::put(col, row, *s ? *s++ : ' ');
    }

    // "SCORE 12" - и на поле, и на экране итога.
    void formatScore(char* buf, uint16_t score) { Text::num(Text::str(buf, "SCORE "), score); }

    // Координата в 1/16 точки → точки (с округлением вниз и для отрицательных).
    int16_t toDots(int16_t v) { return v >> 4; }
} // namespace

// ---------- Игра ----------

void FlappyGame::placePipe(Pipe& p, int16_t x)
{
    p.x = x;
    p.gapY = random(2, FIELD_H - 1 - GAP); // проём не прижат к полу и потолку
    p.passed = false;
}

void FlappyGame::resetRound()
{
    score = 0;
    birdY = (FIELD_H / 2 - 1) * 16;
    birdVy = 0;
    started = false;
    flapLatched = false;
    placePipe(pipes[0], PIPE_FIRST_X * 16);
    placePipe(pipes[1], (PIPE_FIRST_X + PIPE_SPACING) * 16);
}

bool FlappyGame::hitsPipe() const
{
    int16_t by = toDots(birdY);
    for (const Pipe& p : pipes)
    {
        int16_t px = toDots(p.x);
        if (px >= BIRD_X + BIRD_SIZE || px + PIPE_W <= BIRD_X) continue; // не по горизонтали
        if (by < p.gapY || by + BIRD_SIZE > p.gapY + GAP) return true;
    }
    return false;
}

bool FlappyGame::tick(bool flap)
{
    if (flap)
    {
        birdVy = FLAP_V;
        Sound::play(SND_FLAP);
    }
    else if (birdVy < MAX_FALL)
    {
        birdVy += GRAVITY;
    }
    birdY += birdVy;
    if (birdY < 0)
    {
        // потолок не убивает: птица упирается в него, как в оригинале
        birdY = 0;
        birdVy = 0;
    }
    if (birdY > (FIELD_H - BIRD_SIZE) * 16) return false; // пол

    for (Pipe& p : pipes) p.x -= SPEED;
    for (uint8_t i = 0; i < PIPE_COUNT; i++)
    {
        Pipe& p = pipes[i];
        // Ушедшая влево труба переезжает за соседнюю - расстояние между трубами не меняется.
        if (p.x <= -PIPE_W * 16) placePipe(p, pipes[1 - i].x + PIPE_SPACING * 16);
        if (!p.passed && toDots(p.x) + PIPE_W <= BIRD_X)
        {
            p.passed = true;
            score++;
            Sound::play(SND_POINT);
            showScore();
        }
    }
    return !hitsPipe();
}

void FlappyGame::render(bool birdVisible)
{
    canvas.clear();
    for (const Pipe& p : pipes)
    {
        int16_t px = toDots(p.x);
        for (int16_t x = px; x < px + PIPE_W; x++)
        {
            if (x < 0 || x >= FIELD_W) continue;
            for (uint8_t y = 0; y < FIELD_H; y++)
                if (y < p.gapY || y >= p.gapY + GAP) canvas.set(x, y);
        }
    }
    if (birdVisible)
    {
        uint8_t by = toDots(birdY);
        for (uint8_t d = 0; d < BIRD_SIZE * BIRD_SIZE; d++) canvas.set(BIRD_X + d % BIRD_SIZE, by + d / BIRD_SIZE);
    }
    canvas.flush();
}

void FlappyGame::showScore()
{
    char buf[12];
    formatScore(buf, score);
    putText(0, buf);
    putText(1, "");
}

// ---------- Конечный автомат ----------

void FlappyGame::begin() { enter(State::Title); }

void FlappyGame::enter(State s)
{
    state = s;
    phase.reset();
}

void FlappyGame::update()
{
    switch (state)
    {
    case State::Title: updateTitle();
        break;
    case State::Playing: updatePlaying();
        break;
    case State::Crash: updateCrash();
        break;
    case State::GameOver: updateGameOver();
        break;
    }
}

void FlappyGame::updateTitle()
{
    if (phase.step == 0)
    {
        title.begin("FLAPPY BIRD", HISCORE_SLOT, "UP/CLICK: FLAP");
        phase.next();
    }
    switch (title.update())
    {
    case TitleScreen::START: startRound();
        break;
    case TitleScreen::EXIT: requestExit();
        break;
    default: break;
    }
}

void FlappyGame::startRound()
{
    resetRound();
    Display::clear();
    canvas.begin(0, 0);
    putText(0, "GET READY");
    putText(1, "UP: FLAP");
    render();
    Sound::play(SND_START);
    ticker.start();
    enter(State::Playing);
}

void FlappyGame::updatePlaying()
{
    if (Input::anyPressed(Input::UP | Input::CLICK)) flapLatched = true;
    if (!ticker.due(TICK_MS)) return;

    if (!started)
    {
        if (!flapLatched)
        {
            // До первого взмаха птица парит, покачиваясь на точку, трубы стоят.
            birdY = (FIELD_H / 2 - 1 + ((millis() >> 9) & 1)) * 16;
            render();
            return;
        }
        started = true;
        showScore();
    }

    bool alive = tick(flapLatched);
    flapLatched = false;
    // При ударе о пол птица остаётся видна у края
    if (birdY > (FIELD_H - BIRD_SIZE) * 16) birdY = (FIELD_H - BIRD_SIZE) * 16;
    render();
    if (!alive)
    {
        Sound::play(SND_CRASH);
        enter(State::Crash);
    }
}

// Птица мигает на месте удара.
void FlappyGame::updateCrash()
{
    if (phase.elapsed() < 120) return;
    phase.next();
    render(phase.step % 2 == 0);
    if (phase.step == 8) enter(State::GameOver);
}

// Итог и рекорд; на титульный экран - по таймеру или нажатию.
void FlappyGame::updateGameOver()
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
