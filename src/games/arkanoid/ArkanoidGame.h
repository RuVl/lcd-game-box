// Arkanoid (Breakout) на пиксельном холсте 4×2 знакоместа = 20×16 точек.
//
// Поле стоит вертикально: кирпичи сверху, ракетка на нижней строке точек. Так мяч летает
// по длинной оси привычно "вверх-вниз", а ракетку двигают ←/→ - естественно для джойстика.
// Слева от поля - счёт, справа - уровень и жизни.
//
// Управление: ←/→ - ракетка, нажатие стика (или ↑) - запустить мяч.
// Угол отскока зависит от того, куда мяч попал на ракетку; если ракетка в этот момент едет,
// угол становится положе в сторону движения.
// Четыре раскладки кирпичей идут по кругу, с каждым уровнем мяч быстрее.
// Титульный экран: нажатие - старт, удержание стика - выход в меню устройства.
#pragma once

#include "core/Game.h"
#include "core/GameScreens.h"
#include "core/Phase.h"
#include "core/PixelCanvas.h"
#include "core/Ticker.h"

namespace arkanoid
{
    constexpr uint16_t TICK_MS = 20; // шаг физики
    constexpr uint8_t FIELD_W = 20; // поле в точках
    constexpr uint8_t FIELD_H = 16;
    constexpr uint8_t BRICK_COLS = 10; // кирпич 2×1 точки
    constexpr uint8_t BRICK_ROWS = 6;
    constexpr uint8_t BRICK_TOP = 1; // верхняя строка кирпичей (над ней - щель под потолком)
    constexpr uint8_t PADDLE_W = 4;
    constexpr uint8_t PADDLE_Y = FIELD_H - 1;
    constexpr uint8_t PADDLE_FIRST_WAIT = 8; // ракетка: после первого шага пауза в шагах физики,
    constexpr uint8_t PADDLE_EVERY = 3; // потом шаг раз в столько шагов
    // Скорость мяча в 1/256 точки за шаг: 40 ≈ 8 точек/с на первом уровне.
    constexpr uint8_t SPEED_BASE = 40;
    constexpr uint8_t SPEED_STEP = 6;
    constexpr uint8_t SPEED_MAX = 100;
    constexpr uint8_t START_LIVES = 3;
    constexpr uint8_t HISCORE_SLOT = 4; // слот рекорда в EEPROM
} // namespace arkanoid

class ArkanoidGame : public Game
{
public:
    void begin() override;
    void update() override;

private:
    enum class State : uint8_t { Title, Playing, Pause, GameOver };

    uint8_t brickCount;

    // Мяч: координаты и скорость в 1/256 точки, чтобы летать под любым углом.
    uint16_t ballX, ballY;
    int16_t velX, velY;
    uint8_t drawnX, drawnY; // где мяч нарисован сейчас
    bool stuck; // лежит на ракетке и ждёт запуска
    uint8_t ballSpeed; // длина вектора скорости, растёт с уровнем

    uint8_t paddleX;
    int8_t paddleDir; // куда ракетка двигалась в этом шаге
    int8_t heldDir; // куда наклонён стик (для паузы перед автоповтором)
    uint8_t paddleWait;

    uint8_t level; // с нуля
    uint8_t lives;
    uint32_t score;

    State state;
    Phase phase;
    Ticker ticker; // шаг игры
    bool launchLatched; // нажатие между шагами физики не теряется
    bool hudDirty;

    // Холст - последним полем: на AVR к первым 64 байтам объекта обращение короче,
    // пусть там лежат часто используемые переменные.
    // Кирпичи хранятся прямо в точках холста: мяч и ракетка в зону кирпичей не рисуются,
    // поэтому горящая точка там - это и есть целый кирпич.
    TitleScreen title;
    ResultScreen result;
    PixelCanvas canvas;

    bool brickAt(uint8_t x, uint8_t y) const;
    void hitBrick(uint8_t x, uint8_t y);
    void setDirection(uint8_t dir);
    void placeBallOnPaddle();
    void movePaddle(int8_t dx);
    bool moveBall(); // false - мяч упал
    void render();
    void drawHud();

    void enter(State s);
    void startLevel();
    void startServe();
    void updateTitle();
    void updatePlaying();
    void updatePause();
    void updateGameOver();
};
