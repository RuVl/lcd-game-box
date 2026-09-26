#include "core/Network.h"

#if GAMEBOX_HAS_WIFI

#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

#include "core/Display.h"
#include "core/Sound.h"
#include "core/Text.h"

namespace Network {

namespace {
bool otaStarted;

void startOta() {
  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.onStart([] {
    Sound::stop();
    Display::clear();
    Display::printCentered(0, "OTA UPDATE");
  });
  ArduinoOTA.onProgress([](unsigned int done, unsigned int total) {
    char buf[8];
    Text::chr(Text::num(buf, done * 100 / total), '%');
    Display::printCentered(1, buf);
  });
  ArduinoOTA.onEnd([] { Display::printCentered(1, "RESTART"); });
  ArduinoOTA.onError([](ota_error_t) { Display::printCentered(1, "OTA ERROR"); });
  ArduinoOTA.begin();
  otaStarted = true;
}
}  // namespace

void begin() {
  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOSTNAME);
  WiFi.setAutoReconnect(true);
  WiFi.begin();  // к последней запомненной сети, если она есть
}

void update() {
  if (!connected()) return;
  if (!otaStarted) startOta();  // mDNS и OTA поднимаются, когда уже есть IP
  ArduinoOTA.handle();
  MDNS.update();
}

bool connected() { return WiFi.status() == WL_CONNECTED; }

String ip() { return WiFi.localIP().toString(); }

void connect(const char *ssid, const char *pass) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);  // ESP8266 запоминает сеть во Flash
}

}  // namespace Network

#endif
