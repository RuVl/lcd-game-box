// Интерфейс игры. Лаунчер создаёт выбранную игру в общей области памяти (см. GameRegistry.h),
// вызывает begin() и затем update() на каждом проходе loop(), пока игра не попросит выйти.
//
// Правила для игр:
//   - update() не блокирует: никаких delay() и ожиданий в цикле, время - через millis();
//   - ввод - через Input, звук - через Sound, экран - через Display;
//   - все 8 пользовательских символов дисплея принадлежат запущенной игре;
//   - состояние игры - поля её класса, а не глобальные переменные: тогда память занята
//     только пока игра запущена.
#pragma once

#include <stddef.h>

class Game
{
public:
    virtual ~Game() = default;

    // Игры создаются в общей области памяти, не в куче. Пустой operator delete нужен,
    // чтобы виртуальный деструктор не тянул в прошивку malloc/free (~600 байт Flash).
    static void operator delete(void*, size_t)
    {
    }

    virtual void begin() = 0;
    virtual void update() = 0;

    bool exitRequested() const { return exitRequested_; }

protected:
    void requestExit() { exitRequested_ = true; }

private:
    bool exitRequested_ = false;
};
