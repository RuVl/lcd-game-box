// Космический шутер: корабль слева, враги волнами летят справа.
//
// Каждое знакоместо поделено по высоте пополам, поэтому дорожек по вертикали 4 (0 - верх
// верхней строки … 3 - низ нижней). Символ клетки собирается на лету из двух половинок
// (верхняя дорожка + нижняя), и такие пары раздаются 8 символам CGRAM по мере надобности.
//
// Управление: ↑/↓ - дорожка, ←/→ - колонки 0…3, нажатие стика - выстрел, удержание - автоогонь.
// Враги: тарелка летит прямо, зигзаг меняет дорожку, пушка медленная и стреляет в ответ.
// Каждая 4-я волна - босс (2×2 дорожки): следит за дорожкой корабля, стреляет в неё зарядами,
// которые не сбить (только увернуться), мигает и таранит поле по своим дорожкам, зовёт подмогу.
// Титульный экран: нажатие - старт, удержание - выход в меню устройства.
#pragma once

#include "core/Game.h"
#include "core/GameScreens.h"
#include "core/Phase.h"
#include "core/Ticker.h"

class ShooterGame : public Game
{
public:
    void begin() override;
    void update() override;

private:
    enum class State : uint8_t { Title, WaveIntro, Playing, ShipLost, GameOver };

    static constexpr uint8_t MAX_OBJS = 14;
    static constexpr uint8_t MAX_ENEMIES = 6;
    static constexpr uint8_t MAX_SHOTS = 3;

    // Всё, что летает по дорожкам: выстрелы, враги, их пули, взрывы.
    struct Obj
    {
        uint8_t kind; // shooter::Kind, K_NONE - слот свободен
        int8_t x;
        uint8_t lane;
        int8_t dir; // зигзаг: куда сдвинется дорожка
        uint8_t t; // враг - шаги до хода, взрыв - сколько ещё гореть
    };

    // Босс висит справа и держится у дорожки корабля (HOVER), время от времени мигает (WINDUP),
    // таранит влево через всё поле по своим двум дорожкам (CHARGE) и возвращается (BACK).
    enum BossMode : uint8_t { B_HOVER, B_WINDUP, B_CHARGE, B_BACK };

    struct Boss
    {
        bool on;
        int8_t x;
        uint8_t lane; // верхняя из двух его дорожек: 0…2
        uint8_t hp;
        uint8_t flash; // после попадания и перед тараном мигает
        uint8_t t; // шаги до хода
        uint8_t mode; // BossMode
        uint8_t level; // сложность: 1…4 (какой по счёту босс, дальше не растёт)
        uint8_t fire; // шаги до выстрела
        uint8_t charge; // шаги до тарана (в WINDUP - до рывка)
    };

    // --- игра ---
    uint32_t score;
    uint8_t lives;
    uint8_t wave;
    uint8_t shipX, shipLane;
    uint8_t invuln;
    uint8_t fireCool;
    bool fireLatched; // нажатие между шагами не теряется
    uint8_t toSpawn, spawnCool;
    Obj objs[MAX_OBJS];
    Boss boss;
    uint16_t tickCount;

    // --- экран ---
    uint8_t slotPair[8]; // какая пара половинок лежит в символе CGRAM (0 - свободен)

    // --- автомат состояний ---
    State state;
    Phase phase;
    Ticker ticker; // шаг игры
    TitleScreen title;
    ResultScreen result;

    // Параметры волны
    bool bossWave() const;
    uint8_t movePeriod() const;

    // Отрисовка
    int8_t loadedSlot(uint8_t pair) const; // -1 - пара не загружена
    uint8_t slotFor(uint8_t pair, uint8_t& keep);
    void render(bool shipBoom = false);
    void statusScreen(const char* title);

    // Объекты
    Obj* add(uint8_t kind, int8_t x, uint8_t lane); // nullptr - нет свободного слота
    Obj* find(int8_t x, uint8_t lane, uint8_t kindLo, uint8_t kindHi);
    uint8_t count(uint8_t kindLo, uint8_t kindHi) const;
    void explode(Obj& o);
    bool shotHits(int8_t x, uint8_t lane);
    bool bossCovers(int8_t x, uint8_t lane) const;
    void damageBoss();
    void moveBoss();
    bool shipCrash();
    bool gameTick(); // true - корабль подбит

    // Состояния
    void enter(State s);
    void resume();
    void updateTitle();
    void updateWaveIntro();
    void updatePlaying();
    void updateShipLost();
    void updateGameOver();
};
