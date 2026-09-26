#include "games/racing/RacingGame.h"

#include "core/Config.h"
#include "core/Display.h"
#include "core/Input.h"
#include "core/Sound.h"
#include "games/racing/RacingData.h"

using namespace racing;

// ---------- Дорога ----------

void RacingGame::addScore(uint8_t points)
{
    uint16_t s = run.score + points;
    run.score = s < run.score ? 0xFFFF : s;
}

// Новая колонка справа. Машины идут "рядами" через пустые промежутки (не меньше двух
// колонок) - в промежутке можно сменить полосу. Гарантия проезда: от каждой свободной
// полосы прошлого ряда до свободной полосы нового - не больше одной полосы.
uint8_t RacingGame::newColumn()
{
    if (run.gapLeft)
    {
        run.gapLeft--;
        return 0;
    }
    Level lv;
    memcpy_P(&lv, &LEVELS[run.level], sizeof(lv));
    run.gapLeft = lv.gapMin + random(lv.gapRand + 1);
    uint8_t n = 1 + random(lv.maxCars);
    uint8_t cars = 0;
    while (n)
    {
        uint8_t b = 1 << random(LANES);
        if (!(cars & b))
        {
            cars |= b;
            n--;
        }
    }
    for (uint8_t f = 0; f < LANES; f++)
    {
        if (run.prevCars & (1 << f)) continue;
        uint8_t reach = ((7 << f) >> 1) & 0x0F; // полосы f-1…f+1
        if ((cars & reach) == reach) cars &= ~(1 << f);
    }
    run.prevCars = cars;
    return cars;
}

bool RacingGame::hitsPlayer() const { return run.road[PLAYER_COL] & (1 << run.lane); }

// Поток сдвигается на колонку. Колонка, которую игрок только что проехал, - проверка
// на "впритирку": машина в соседней полосе даёт бонус.
void RacingGame::scrollTraffic()
{
    uint8_t me = 1 << run.lane;
    if (run.road[PLAYER_COL] & ((me << 1) | (me >> 1)) & 0x0F)
    {
        addScore(NEAR_MISS_BONUS * run.speed);
        if (!Sound::playing()) Sound::play(SND_NEAR_MISS);
    }
    for (uint8_t i = 0; i < ROAD_COLS - 1; i++) run.road[i] = run.road[i + 1];
    run.road[ROAD_COLS - 1] = newColumn();
}

// ---------- Шаг игры ----------

bool RacingGame::gameTick()
{
    // Крейсерская передача (→/←): от минимальной до MAX_GEAR
    int8_t g = run.gear + speedLatch;
    speedLatch = 0;
    if (g >= run.minGear && g <= MAX_GEAR && g != run.gear)
    {
        Sound::play(g > run.gear ? SND_GEAR_UP : SND_GEAR_DOWN);
        run.gear = g;
        run.shiftWait = 0; // ручная смена - сразу
    }

    // Газ: пока стик нажат, тянемся к MAX_GEAR, иначе - к крейсерской
    bool gas = Input::held(Input::CLICK);
    if (gas && !run.gas)
    {
        Sound::play(SND_GAS);
        run.shiftWait = 0;
    }
    run.gas = gas;
    uint8_t target = gas ? MAX_GEAR : run.gear;
    if (run.shiftWait) run.shiftWait--;
    if (run.speed == target)
    {
        run.shiftWait = 0;
    }
    else if (!run.shiftWait)
    {
        bool up = run.speed < target;
        run.speed += up ? 1 : -1;
        run.shiftWait = up ? ACCEL_TICKS : DECEL_TICKS;
    }

    // Полоса: въехать в машину сбоку - тоже авария
    int8_t l = run.lane + laneLatch;
    laneLatch = 0;
    if (l >= 0 && l < LANES)
    {
        run.lane = l;
        if (hitsPlayer()) return false;
    }

    // Разметка едет со скоростью игрока, машины - на четверть медленнее
    uint8_t rate = pgm_read_byte(&ROAD_RATE[run.speed - 1]);
    run.roadAcc += rate;
    if (run.roadAcc >= 256)
    {
        run.roadAcc -= 256;
        run.markPhase = run.markPhase ? run.markPhase - 1 : MARK_EVERY - 1; // штрихи едут влево
        addScore(1);
        if (++run.levelDist == DIST_PER_LEVEL && run.level < MAX_LEVEL)
        {
            run.levelDist = 0;
            run.level++;
        }
        // Минимальная передача: каждый следующий подъём - на MIN_GROW колонок дальше
        if (run.minGear < MAX_GEAR && --run.minLeft == 0)
        {
            run.minLeft = MIN_FIRST + MIN_GROW * run.minGear;
            run.minGear++;
            if (run.gear < run.minGear) run.gear = run.minGear;
            if (run.speed < run.minGear)
            {
                run.speed = run.minGear;
                Sound::play(SND_GEAR_UP);
            }
        }
    }
    run.trafficAcc += rate - rate / 4;
    if (run.trafficAcc >= 256)
    {
        run.trafficAcc -= 256;
        scrollTraffic();
        if (hitsPlayer()) return false;
    }
    return true;
}

// ---------- Отрисовка ----------

// Во время аварии на месте игрока - взрыв (он в символе разметки, разметка скрыта).
void RacingGame::render(bool showPlayer)
{
    bool crashed = state == State::Crash;
    uint8_t mark = run.markPhase; // через сколько колонок следующий штрих разметки
    for (uint8_t col = 0; col < ROAD_COLS; col++)
    {
        for (uint8_t row = 0; row < Display::ROWS; row++)
        {
            uint8_t cell = (run.road[col] >> (row * 2)) & 3; // бит 0 - верхняя половина, бит 1 - нижняя
            uint8_t c;
            if (showPlayer && col == PLAYER_COL && (run.lane >> 1) == row)
            {
                if (crashed)
                    c = G_MARK;
                else if (!(run.lane & 1))
                    c = (cell & 2) ? G_ME_TOP_CAR : G_ME_TOP;
                else
                    c = (cell & 1) ? G_ME_BOT_CAR : G_ME_BOT;
            }
            else if (cell)
            {
                c = cell - 1; // G_CAR_TOP, G_CAR_BOT, G_CAR_BOTH
            }
            else
            {
                c = !crashed && mark == 0 ? G_MARK : ' ';
            }
            Display::put(col, row, c);
        }
        mark = mark ? mark - 1 : MARK_EVERY - 1;
    }
}

// Справа: счёт (4 цифры); ниже - передача, ">" пока нажат газ, "_" и минимальная передача.
void RacingGame::renderHud()
{
    uint16_t v = run.score < 9999 ? run.score : 9999;
    for (uint8_t i = HUD_COL + 3; i >= HUD_COL; i--)
    {
        Display::put(i, 0, '0' + v % 10);
        v /= 10;
    }
    Display::put(HUD_COL, 1, '0' + run.speed);
    Display::put(HUD_COL + 1, 1, run.gas ? '>' : ' ');
    Display::put(HUD_COL + 2, 1, '_');
    Display::put(HUD_COL + 3, 1, '0' + run.minGear);
}

// ---------- Конечный автомат ----------

void RacingGame::begin() { enter(State::Title); }

void RacingGame::enter(State s)
{
    state = s;
    phase.reset();
}

void RacingGame::update()
{
    switch (state)
    {
    case State::Title: updateTitle();
        break;
    case State::Start: updateStart();
        break;
    case State::Playing: updatePlaying();
        break;
    case State::Crash: updateCrash();
        break;
    case State::GameOver: updateGameOver();
        break;
    }
}

// Нажатие - старт, удержание - выход в меню устройства.
void RacingGame::updateTitle()
{
    if (phase.step == 0)
    {
        for (uint8_t i = 0; i < GLYPH_COUNT; i++) Display::loadGlyph(i, GLYPHS[i]); // взрыв → разметка
        title.begin("RACING", HISCORE_SLOT);
        Display::put(3, 0, G_ME_BOT);
        Display::put(12, 0, G_CAR_BOT);
        phase.next();
    }
    switch (title.update())
    {
    case TitleScreen::START:
        memset(&run, 0, sizeof(run));
        run.gapLeft = 4; // первые машины - не сразу
        run.lane = 1;
        run.speed = run.gear = run.minGear = 1;
        run.minLeft = MIN_FIRST;
        Display::clear();
        enter(State::Start); // до render(): он смотрит на state
        render();
        renderHud();
        Sound::play(SND_START);
        break;
    case TitleScreen::EXIT: requestExit();
        break;
    default: break;
    }
}

// Обратный отсчёт: три коротких сигнала и длинный - старт.
void RacingGame::updateStart()
{
    if (Sound::playing()) return;
    laneLatch = speedLatch = 0;
    ticker.start();
    enter(State::Playing);
}

void RacingGame::updatePlaying()
{
    if (Input::repeat(Input::UP)) laneLatch = -1;
    if (Input::repeat(Input::DOWN)) laneLatch = 1;
    if (Input::repeat(Input::RIGHT)) speedLatch = 1;
    if (Input::repeat(Input::LEFT)) speedLatch = -1;
    if (!ticker.due(TICK_MS)) return;

    if (!gameTick())
    {
        Display::loadGlyph(G_MARK, SPR_BOOM);
        Sound::play(SND_CRASH);
        enter(State::Crash);
    }
    render();
    renderHud();
}

// Взрыв мигает, пока звучит авария.
void RacingGame::updateCrash()
{
    if (phase.elapsed() < 120) return;
    phase.next();
    render(phase.step % 2 == 0);
    if (phase.step >= 8 && !Sound::playing()) enter(State::GameOver);
}

// Итог и рекорд; на титульный экран - по таймеру или нажатию.
void RacingGame::updateGameOver()
{
    if (phase.step == 0)
    {
        result.begin("GAME OVER", run.score, HISCORE_SLOT);
        Sound::play(SND_GAME_OVER);
        phase.next();
        return;
    }
    bool hadRecord = result.newRecord();
    if (result.update()) enter(State::Title);
    else if (!hadRecord && result.newRecord()) Sound::play(SND_RECORD);
}
