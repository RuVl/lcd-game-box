#include "system/SettingsApp.h"

#include "core/Config.h"
#if !defined(GAMEBOX_HAS_WIFI)
#error "core/Config.h is required: GAMEBOX_* macros select what is built"
#endif

#if GAMEBOX_HAS_WIFI

#include <AutoOTA.h>
#include <SimplePortal.h>

#include "core/Display.h"
#include "core/Input.h"
#include "core/Network.h"
#include "core/Text.h"

namespace
{
    // Манифест обновлений: project.json в корне ветки main репозитория.
    constexpr const char* OTA_MANIFEST = "RuVl/lcd-game-box";
    constexpr uint32_t CONNECT_TIMEOUT_MS = 20000;
    constexpr uint32_t MESSAGE_MS = 2500;

    const char* const ITEMS[] = {"WI-FI SETUP", "CHECK UPDATE", "ABOUT"};
    constexpr uint8_t ITEM_COUNT = sizeof(ITEMS) / sizeof(ITEMS[0]);

    AutoOTA& ota()
    {
        static AutoOTA instance(GAMEBOX_VERSION, OTA_MANIFEST);
        return instance;
    }
} // namespace

void SettingsApp::begin() { enter(State::Menu); }

void SettingsApp::enter(State s)
{
    state = s;
    phase.reset();
}

void SettingsApp::drawMenu()
{
    Display::clear();
    Display::printCentered(0, "SETTINGS");
    char row[Display::COLS + 1];
    char* p = Text::str(Text::str(row, "< "), ITEMS[item]);
    while (p < row + Display::COLS - 2) p = Text::chr(p, ' ');
    Text::str(p, " >");
    Display::printRow(1, row);
}

void SettingsApp::showMessage(const char* top, const char* bottom)
{
    Display::clear();
    Display::printCentered(0, top);
    Display::printCentered(1, bottom);
    enter(State::Message);
}

void SettingsApp::update()
{
    switch (state)
    {
    case State::Menu: updateMenu();
        break;
    case State::Portal: updatePortal();
        break;
    case State::Connecting: updateConnecting();
        break;
    case State::Checking: updateChecking();
        break;
    case State::UpdateFound: updateUpdateFound();
        break;
    case State::Installing: updateInstalling();
        break;
    case State::Message: updateMessage();
        break;
    }
}

void SettingsApp::updateMenu()
{
    if (phase.step == 0)
    {
        drawMenu();
        phase.next();
    }
    if (Input::clickHeldMs() >= config::HOLD_TO_EXIT_MS)
    {
        requestExit();
        return;
    }
    if (Input::repeat(Input::LEFT | Input::RIGHT))
    {
        item = Input::held(Input::LEFT) ? (item + ITEM_COUNT - 1) % ITEM_COUNT : (item + 1) % ITEM_COUNT;
        drawMenu();
    }
    if (Input::released(Input::CLICK) && Input::lastClickMs() < config::HOLD_TO_EXIT_MS) activate();
}

void SettingsApp::activate()
{
    switch (item)
    {
    case 0: // WI-FI SETUP: точка доступа с формой логина и пароля
        portalStart();
        Display::clear();
        Display::printCentered(0, "WIFI: " SP_AP_NAME);
        Display::printCentered(1, "OPEN 192.168.1.1");
        enter(State::Portal);
        break;
    case 1: // CHECK UPDATE
        if (!Network::connected())
        {
            showMessage("NO WI-FI", "SETUP IT FIRST");
        }
        else
        {
            showMessage("CHECKING...", "v" GAMEBOX_VERSION);
            enter(State::Checking); // сам запрос - на следующем проходе, надпись уже на экране
        }
        break;
    case 2: // ABOUT
        showMessage("LCD GAME BOX v" GAMEBOX_VERSION,
                    Network::connected() ? Network::ip().c_str() : "NO WI-FI");
        break;
    }
}

void SettingsApp::updatePortal()
{
    if (Input::clickHeldMs() >= config::HOLD_TO_EXIT_MS)
    {
        portalStop();
        Input::suppressUntilRelease();
        enter(State::Menu);
        return;
    }
    if (!portalTick()) return;
    if (portalStatus() == SP_SUBMIT)
    {
        Network::connect(portalCfg.SSID, portalCfg.pass);
        Display::clear();
        Display::printCentered(0, "CONNECTING TO");
        Display::printCentered(1, portalCfg.SSID);
        enter(State::Connecting);
    }
    else
    {
        enter(State::Menu); // "выход" на странице портала или таймаут
    }
}

void SettingsApp::updateConnecting()
{
    if (Network::connected())
    {
        showMessage("CONNECTED", Network::ip().c_str());
    }
    else if (phase.elapsed() >= CONNECT_TIMEOUT_MS)
    {
        showMessage("CONNECT FAILED", "CHECK PASSWORD");
    }
}

void SettingsApp::updateChecking()
{
    String version;
    if (ota().checkUpdate(&version))
    {
        strncpy(newVersion, version.c_str(), sizeof(newVersion) - 1);
        newVersion[sizeof(newVersion) - 1] = '\0';
        Display::clear();
        char top[Display::COLS + 1];
        Text::str(Text::str(top, "NEW v"), newVersion);
        Display::printCentered(0, top);
        Display::printCentered(1, "CLICK: INSTALL");
        enter(State::UpdateFound);
    }
    else if (ota().getError() == AutoOTA::Error::NoUpdates)
    {
        showMessage("UP TO DATE", "v" GAMEBOX_VERSION);
    }
    else
    {
        char bottom[Display::COLS + 1];
        Text::num(Text::str(bottom, "ERROR "), (uint8_t)ota().getError());
        showMessage("UPDATE CHECK", bottom);
    }
}

void SettingsApp::updateUpdateFound()
{
    if (Input::clickHeldMs() >= config::HOLD_TO_EXIT_MS)
    {
        Input::suppressUntilRelease();
        enter(State::Menu);
        return;
    }
    if (Input::released(Input::CLICK))
    {
        Display::clear();
        Display::printCentered(0, "UPDATING...");
        Display::printCentered(1, "DO NOT POWER OFF");
        enter(State::Installing);
    }
}

void SettingsApp::updateInstalling()
{
    // Скачивание и запись прошивки; при успехе плата перезагрузится и сюда не вернётся.
    if (!ota().updateNow())
    {
        char bottom[Display::COLS + 1];
        Text::num(Text::str(bottom, "ERROR "), (uint8_t)ota().getError());
        showMessage("UPDATE FAILED", bottom);
    }
}

void SettingsApp::updateMessage()
{
    if (phase.elapsed() >= MESSAGE_MS || Input::pressed(Input::CLICK))
    {
        Input::suppressUntilRelease();
        enter(State::Menu);
    }
}

#endif
