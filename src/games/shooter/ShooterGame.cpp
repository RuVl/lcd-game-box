#include "games/shooter/ShooterGame.h"

#include "core/Config.h"
#include "core/Display.h"
#include "core/Input.h"
#include "core/Sound.h"
#include "core/Text.h"
#include "games/shooter/ShooterData.h"

using namespace shooter;

namespace
{
    constexpr uint8_t NO_SLOT = 0xFF;
} // namespace

// ---------- Параметры волны ----------

bool ShooterGame::bossWave() const { return wave % BOSS_EVERY == 0; }

// Через сколько шагов враг сдвигается на клетку: 5 на первых волнах, 2 с девятой.
uint8_t ShooterGame::movePeriod() const { return 5 - (wave < 9 ? wave : 9) / 3; }

// ---------- Отрисовка ----------
// Пара = вид на верхней дорожке клетки << 4 | вид на нижней. Каждой паре на экране нужен
// свой символ CGRAM; пара остаётся в символе, пока она видна, поэтому перезагрузок мало.

int8_t ShooterGame::loadedSlot(uint8_t pair) const
{
    for (uint8_t s = 0; s < 8; s++)
        if (slotPair[s] == pair) return s;
    return -1;
}

// Символ для пары: уже загруженный или любой, не занятый в этом кадре (keep).
uint8_t ShooterGame::slotFor(uint8_t pair, uint8_t& keep)
{
    int8_t s = loadedSlot(pair);
    if (s < 0)
    {
        for (s = 0; s < 8 && (keep & (1 << s)); s++)
        {
        }
        if (s == 8) return NO_SLOT;
        uint8_t buf[8];
        memcpy_P(buf, HALF[pair >> 4], 4);
        memcpy_P(buf + 4, HALF[pair & 15], 4);
        Display::lcd.createChar(s, buf);
        slotPair[s] = pair;
    }
    keep |= 1 << s;
    return s;
}

void ShooterGame::render(bool shipBoom)
{
    uint8_t cells[Display::ROWS * Display::COLS] = {};
    auto draw = [&cells](int8_t x, uint8_t lane, uint8_t kind)
    {
        if (x < 0 || x >= Display::COLS) return;
        uint8_t& c = cells[(lane >> 1) * Display::COLS + x];
        c = (lane & 1) ? (c & 0xF0) | kind : (c & 0x0F) | kind << 4;
    };
    for (const Obj& o : objs)
        if (o.kind) draw(o.x, o.lane, o.kind);
    if (boss.on && !(boss.flash & 1))
        for (uint8_t i = 0; i < 4; i++) draw(boss.x + (i >> 1), boss.lane + (i & 1), K_BOSS_LT + i);
    if (shipBoom)
        draw(shipX, shipLane, K_BOOM);
    else if (!(invuln & 2)) // неуязвимый корабль мигает
        draw(shipX, shipLane, K_SHIP);

    // Сначала закрепить нужные пары, которые уже загружены, потом раздать остальные символы.
    uint8_t keep = 0;
    for (uint8_t c : cells)
    {
        int8_t s = c ? loadedSlot(c) : -1;
        if (s >= 0) keep |= 1 << s;
    }
    // Слева направо: если символов не хватит, буквой станет дальний от корабля край.
    for (uint8_t col = 0; col < Display::COLS; col++)
        for (uint8_t row = 0; row < Display::ROWS; row++)
        {
            uint8_t c = cells[row * Display::COLS + col];
            uint8_t ch = ' ';
            if (c)
            {
                ch = slotFor(c, keep);
                if (ch == NO_SLOT) ch = pgm_read_byte(&FALLBACK[(c >> 4) ? (c >> 4) : (c & 15)]);
            }
            Display::put(col, row, ch);
        }
}

// Заголовок и строка "корабль × жизни, счёт".
void ShooterGame::statusScreen(const char* title)
{
    Display::clear();
    Display::printCentered(0, title);
    char buf[17];
    Text::num(Text::str(Text::chr(Text::str(buf, "  x"), '0' + lives), "  "), score, 6); // "  x3  000120"
    Display::printRow(1, buf);
    uint8_t keep = 0;
    Display::put(1, 1, slotFor(K_SHIP << 4 | K_SHIP, keep));
}

// ---------- Объекты ----------

ShooterGame::Obj* ShooterGame::add(uint8_t kind, int8_t x, uint8_t lane)
{
    for (Obj& o : objs)
        if (!o.kind)
        {
            o = {kind, x, lane, (int8_t)(lane < 2 ? 1 : -1), BOOM_TICKS};
            return &o;
        }
    return nullptr;
}

// Объект одного из видов kindLo…kindHi в клетке (x, lane).
ShooterGame::Obj* ShooterGame::find(int8_t x, uint8_t lane, uint8_t kindLo, uint8_t kindHi)
{
    for (Obj& o : objs)
        if (o.kind >= kindLo && o.kind <= kindHi && o.x == x && o.lane == lane) return &o;
    return nullptr;
}

uint8_t ShooterGame::count(uint8_t kindLo, uint8_t kindHi) const
{
    uint8_t n = 0;
    for (const Obj& o : objs)
        if (o.kind >= kindLo && o.kind <= kindHi) n++;
    return n;
}

// Враг или пуля взрывается. За врагов очки: 10, 20, 30.
void ShooterGame::explode(Obj& o)
{
    if (o.kind <= K_GUN) score += 10 * (o.kind - K_SAUCER + 1);
    o.kind = K_BOOM;
    o.t = BOOM_TICKS;
    Sound::play(SND_BOOM);
}

// Выстрел оказался в клетке (x, lane): true - во что-то попал (выстрелы сбивают и пули,
// а о заряд босса просто гаснут).
bool ShooterGame::shotHits(int8_t x, uint8_t lane)
{
    Obj* o = find(x, lane, K_SAUCER, K_BOLT);
    if (o)
    {
        if (o->kind != K_BOLT) explode(*o);
        return true;
    }
    if (!bossCovers(x, lane)) return false;
    damageBoss();
    return true;
}

bool ShooterGame::bossCovers(int8_t x, uint8_t lane) const
{
    return boss.on && (uint8_t)(x - boss.x) < 2 && (uint8_t)(lane - boss.lane) < 2;
}

void ShooterGame::damageBoss()
{
    if (boss.flash < 3) boss.flash = 3; // мигание перед тараном не обрывать
    if (--boss.hp)
    {
        Sound::play(SND_BOSS_HIT);
        return;
    }
    boss.on = false;
    toSpawn = 0; // подмога больше не прилетает
    score += 200UL * (wave / BOSS_EVERY);
    for (uint8_t i = 0; i < 4; i++) add(K_BOOM, boss.x + (i & 1), boss.lane + (i >> 1));
    Sound::play(SND_BOSS_DOWN);
}

// Босс выезжает справа и держится у дорожки корабля, но с запозданием. Стреляет в дорожку
// корабля зарядами, иногда очередью или вместе с пулей в другую свою дорожку (её можно сбить).
// Время от времени мигает и таранит влево по своим двум дорожкам - надо уйти с них.
// Каждый следующий босс (level = 1…4) ходит, стреляет и таранит чаще.
void ShooterGame::moveBoss()
{
    if (!boss.on) return;
    if (boss.flash) boss.flash--;
    uint8_t m = boss.level;
    uint8_t aim = shipLane < boss.lane ? boss.lane : shipLane > boss.lane + 1 ? boss.lane + 1 : shipLane;
    switch (boss.mode)
    {
    case B_HOVER:
        if (++boss.t >= 6 - m)
        {
            // ход раз в 5…2 шага
            boss.t = 0;
            if (boss.x > BOSS_X)
            {
                boss.x--;
                break;
            }
            if (random(3)) boss.lane += (shipLane > aim) - (shipLane < aim);
        }
        if (boss.x > BOSS_X) break;
        if (--boss.fire == 0)
        {
            add(K_BOLT, boss.x - 1, aim);
            boss.fire = 20 - 2 * m; // раз в 18…12 шагов
            switch (random(4))
            {
            case 0: add(K_BULLET, boss.x - 1, 2 * boss.lane + 1 - aim);
                break;
            case 1: boss.fire = 4;
                break; // очередь
            }
        }
        if (--boss.charge == 0)
        {
            boss.mode = B_WINDUP;
            boss.charge = boss.flash = WINDUP_TICKS;
        }
        break;
    case B_WINDUP:
        if (--boss.charge == 0) boss.mode = B_CHARGE;
        break;
    case B_CHARGE: // рывок по клетке за шаг до левого края
        if (--boss.x == 0) boss.mode = B_BACK;
        break;
    default: // B_BACK: обратно вдвое медленнее
        if (tickCount & 1) break;
        if (++boss.x < BOSS_X) break;
        boss.mode = B_HOVER;
        boss.charge = 110 - 10 * m; // таран раз в 7…5 с
    }
    // Выстрелы, на которые босс наехал сам
    for (Obj& o : objs)
        if (o.kind == K_SHOT && bossCovers(o.x, o.lane))
        {
            o.kind = K_NONE;
            damageBoss();
        }
}

// Корабль столкнулся с врагом или пулей.
bool ShooterGame::shipCrash()
{
    if (invuln) return false;
    if (bossCovers(shipX, shipLane)) return true;
    Obj* o = find(shipX, shipLane, K_SAUCER, K_BOLT);
    if (o) explode(*o);
    return o;
}

// ---------- Шаг игры ----------
// Каждый ход проверяет клетку, в которую пришёл, поэтому выстрел и враг (или пуля) не могут
// "проскочить" друг сквозь друга, в каком бы порядке ни двигались.
bool ShooterGame::gameTick()
{
    tickCount++;
    if (invuln) invuln--;

    // Огонь: нажатие - сразу, удержание - автоогонь реже
    if (fireCool)
    {
        fireCool--;
    }
    else if (fireLatched || Input::held(Input::CLICK))
    {
        if (count(K_SHOT, K_SHOT) < MAX_SHOTS)
        {
            fireCool = fireLatched ? TAP_FIRE_TICKS : AUTO_FIRE_TICKS;
            if (!Sound::playing()) Sound::play(SND_SHOT); // не перебивать взрывы
            int8_t x = shipX + 1;
            if (!shotHits(x, shipLane)) add(K_SHOT, x, shipLane);
        }
        fireLatched = false;
    }

    uint8_t period = movePeriod();
    for (Obj& o : objs)
    {
        switch (o.kind)
        {
        case K_NONE: continue;
        case K_SHOT:
            if (++o.x >= Display::COLS || shotHits(o.x, o.lane)) o.kind = K_NONE;
            continue;
        case K_BOOM:
            if (--o.t == 0) o.kind = K_NONE;
            continue;
        case K_BULLET:
        case K_BOLT:
            if (tickCount & 1) continue; // пули - через шаг
            break;
        default: // враги; пушка вдвое медленнее
            if (++o.t < (o.kind == K_GUN ? 2 * period : period)) continue;
            o.t = 0;
        }
        if (--o.x < 0)
        {
            // улетел за левый край
            o.kind = K_NONE;
            continue;
        }
        if (o.kind == K_ZIG)
        {
            // В 4 колонках перед кораблём зигзаг обходит его дорожку (не подставляется под выстрелы),
            // а в колонке корабля ныряет в неё - в углу не отсидеться
            uint8_t away = o.lane - o.dir;
            if ((uint8_t)(o.x - shipX) < 5 && away < LANES && (o.x == shipX ? away : o.lane + o.dir) == shipLane)
                o.dir = -o.dir;
            o.lane += o.dir;
            if (o.lane == 0 || o.lane == LANES - 1) o.dir = -o.dir;
        }
        else if (o.kind == K_GUN && o.x > 0 && random(6) < 1 + wave / 3)
        {
            add(K_BULLET, o.x - 1, o.lane);
        }
        Obj* s = find(o.x, o.lane, K_SHOT, K_SHOT);
        if (s)
        {
            s->kind = K_NONE;
            if (o.kind != K_BOLT) explode(o); // заряд босса выстрел только гасит
        }
    }
    moveBoss();

    // Новые враги у правого края. Виды открываются по волнам: 1 - тарелки, 2 - и зигзаги, 3+ - и пушки.
    // Пока жив босс, прилетает подмога - по одному врагу.
    if (spawnCool)
    {
        spawnCool--;
    }
    else if (toSpawn && count(K_SAUCER, K_GUN) < (boss.on ? 1 : MAX_ENEMIES) &&
        add(K_SAUCER + random(wave < 3 ? wave : 3), Display::COLS - 1, random(LANES)))
    {
        toSpawn--;
        spawnCool = 16 - (wave < 10 ? wave : 10); // от 1 с до 0,4 с между врагами
    }
    return shipCrash();
}

// ---------- Конечный автомат ----------

void ShooterGame::begin()
{
    memset(slotPair, 0, sizeof(slotPair)); // в CGRAM ещё символы прошлого экрана
    enter(State::Title);
}

void ShooterGame::enter(State s)
{
    state = s;
    phase.reset();
}

// Вернуться на поле (после заставки волны или потери корабля).
void ShooterGame::resume()
{
    Display::clear();
    fireLatched = false;
    ticker.start();
    render();
    enter(State::Playing);
}

void ShooterGame::update()
{
    switch (state)
    {
    case State::Title: updateTitle();
        break;
    case State::WaveIntro: updateWaveIntro();
        break;
    case State::Playing: updatePlaying();
        break;
    case State::ShipLost: updateShipLost();
        break;
    case State::GameOver: updateGameOver();
        break;
    }
}

// Нажатие стика - старт, удержание - выход в меню устройства.
void ShooterGame::updateTitle()
{
    if (phase.step == 0)
    {
        title.begin("SPACE SHOOTER", HISCORE_SLOT);
        phase.next();
    }
    switch (title.update())
    {
    case TitleScreen::START:
        score = 0;
        lives = START_LIVES;
        wave = 1;
        shipX = 0;
        shipLane = 1;
        invuln = 0;
        enter(State::WaveIntro);
        break;
    case TitleScreen::EXIT: requestExit();
        break;
    default: break;
    }
}

// "WAVE n" и жизни со счётом, затем новая волна.
void ShooterGame::updateWaveIntro()
{
    if (phase.step == 0)
    {
        char buf[17];
        char* p = Text::num(Text::str(buf, "WAVE "), wave);
        if (bossWave()) Text::str(p, ": BOSS!");
        statusScreen(buf);
        Sound::play(bossWave() ? SND_BOSS : SND_WAVE);
        phase.next();
    }
    else if (phase.elapsed() >= 2000)
    {
        memset(objs, 0, sizeof(objs));
        uint8_t n = wave / BOSS_EVERY; // какой по счёту босс
        uint8_t m = n < 4 ? n : 4;
        boss = {
            bossWave(), Display::COLS, 1, (uint8_t)(n < 8 ? 6 + 4 * n : 38), 0, 0, B_HOVER, // 10, 14, 18… попаданий
            m, 1, (uint8_t)(110 - 10 * m)
        };
        toSpawn = bossWave() ? 255 : 8 + 2 * (wave < 11 ? wave : 11); // 10 … 30 врагов
        spawnCool = 0;
        fireCool = 0;
        resume();
    }
}

void ShooterGame::updatePlaying()
{
    if (Input::pressed(Input::CLICK)) fireLatched = true;
    // Корабль двигается сразу, не дожидаясь шага игры
    uint8_t lane = shipLane, x = shipX;
    if (Input::repeat(Input::UP) && lane > 0) lane--;
    if (Input::repeat(Input::DOWN) && lane < LANES - 1) lane++;
    if (Input::repeat(Input::LEFT) && x > 0) x--;
    if (Input::repeat(Input::RIGHT) && x < SHIP_MAX_X) x++;
    bool changed = lane != shipLane || x != shipX;
    shipLane = lane;
    shipX = x;
    bool hit = changed && shipCrash();
    bool ticked = false;
    if (!hit && ticker.due(TICK_MS))
    {
        hit = gameTick();
        ticked = true;
    }
    if (hit)
    {
        Sound::play(SND_HIT);
        enter(State::ShipLost);
        return;
    }
    if (changed || ticked) render();
    // Волна пройдена, когда не осталось врагов, пуль и догорели взрывы
    if (!ticked || toSpawn || boss.on || count(K_SAUCER, K_BOOM)) return;
    score += 20UL * wave; // бонус за волну
    wave++;
    enter(State::WaveIntro);
}

// Взрыв на месте корабля, затем "SHIP LOST" и продолжение той же волны.
void ShooterGame::updateShipLost()
{
    if (phase.step == 0)
    {
        render(true);
        phase.next();
    }
    else if (phase.step == 1)
    {
        if (phase.elapsed() < 1200) return;
        if (--lives == 0)
        {
            enter(State::GameOver);
            return;
        }
        statusScreen("SHIP LOST");
        phase.next();
    }
    else if (phase.elapsed() >= 1500)
    {
        // Рядом с кораблём становится пусто, чтобы не погибнуть сразу снова
        for (Obj& o : objs)
            if (o.x < 6 || o.kind == K_SHOT || o.kind >= K_BULLET) o.kind = K_NONE;
        boss.mode = B_BACK; // таран прерывается, босс уходит на место
        invuln = INVULN_TICKS;
        resume();
    }
}

// Итог и рекорд; на титульный экран - по таймеру или нажатию.
void ShooterGame::updateGameOver()
{
    if (phase.step == 0)
    {
        result.begin("GAME OVER", score, HISCORE_SLOT);
        Sound::play(SND_OVER);
        phase.next();
    }
    else if (result.update())
    {
        enter(State::Title);
    }
}
