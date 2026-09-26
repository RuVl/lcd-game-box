// Настройки (только платы с Wi-Fi): подключение к сети, обновление прошивки с GitHub, версия.
// ←/→ - пункт, нажатие - выбрать, удержание - выход в меню (или отмена действия).
//
// Обновление: AutoOTA скачивает project.json из репозитория и, если версия там другая,
// ставит прошивку по ссылке оттуда (GitHub Releases). Сетевые запросы блокируют
// на пару секунд, поэтому выполняются только по нажатию, с надписью на экране заранее.
#pragma once

#include "core/Config.h"

#if GAMEBOX_HAS_WIFI

#include "core/Game.h"
#include "core/Phase.h"

class SettingsApp : public Game
{
public:
    void begin() override;
    void update() override;

private:
    enum class State : uint8_t { Menu, Portal, Connecting, Checking, UpdateFound, Installing, Message };

    State state;
    Phase phase;
    uint8_t item;
    char newVersion[12];

    void enter(State s);
    void drawMenu();
    void showMessage(const char* top, const char* bottom);
    void activate();

    void updateMenu();
    void updatePortal();
    void updateConnecting();
    void updateChecking();
    void updateUpdateFound();
    void updateInstalling();
    void updateMessage();
};

#endif
