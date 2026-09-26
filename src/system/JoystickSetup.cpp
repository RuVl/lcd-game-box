#include "system/JoystickSetup.h"

#include "core/Config.h"
#include "core/Display.h"
#include "core/Input.h"
#include "core/Storage.h"
#include "core/Text.h"

namespace {
const char *const NAMES[] = {"SENSITIVITY", "DEBOUNCE"};
}  // namespace

void JoystickSetup::begin() {
  sensitivity = Storage::loadSetting(Storage::JOY_SENSITIVITY, config::DEFAULT_SENSITIVITY);
  debounce = Storage::loadSetting(Storage::JOY_DEBOUNCE, config::DEFAULT_DEBOUNCE);
  Display::clear();
  drawSetting();
}

// «SENSITIVITY < 5>» / «DEBOUNCE   <30ms>»
void JoystickSetup::drawSetting() {
  char row[Display::COLS + 1];
  char *p = Text::str(row, NAMES[item]);
  uint8_t value = item == 0 ? sensitivity : debounce;
  uint8_t width = item == 0 ? 2 : 3;  // «< 5>» или «< 30ms>», до «<100ms>»
  uint8_t tail = item == 0 ? 4 : 7;
  while (p < row + Display::COLS - tail) p = Text::chr(p, ' ');
  p = Text::num(Text::chr(p, '<'), item == 0 ? value : value * 10, width, ' ');
  if (item == 1) p = Text::str(p, "ms");
  Text::chr(p, '>');
  Display::printRow(1, row);
}

// «X 512Y 498 LRUDC»: точка — направление не нажато.
void JoystickSetup::drawLive() {
  char row[Display::COLS + 1];
  char *p = Text::num(Text::chr(row, 'X'), Input::rawX(), 4, ' ');
  p = Text::chr(Text::num(Text::chr(p, 'Y'), Input::rawY(), 4, ' '), ' ');
  const char flags[] = "LRUDC";
  for (uint8_t i = 0; i < 5; i++) p = Text::chr(p, Input::held(1 << i) ? flags[i] : '.');
  Display::printRow(0, row);
}

void JoystickSetup::update() {
  if (Input::clickHeldMs() >= config::HOLD_TO_EXIT_MS) {
    Storage::saveSetting(Storage::JOY_SENSITIVITY, sensitivity);
    Storage::saveSetting(Storage::JOY_DEBOUNCE, debounce);
    requestExit();
    return;
  }
  if (Input::released(Input::CLICK) && Input::lastClickMs() < config::HOLD_TO_EXIT_MS) {
    item ^= 1;
    drawSetting();
  }
  int8_t d = Input::repeat(Input::RIGHT) ? 1 : Input::repeat(Input::LEFT) ? -1 : 0;
  if (d) {
    uint8_t &v = item == 0 ? sensitivity : debounce;
    v = constrain(v + d, 1, 10);
    if (item == 0)
      Input::setSensitivity(v);
    else
      Input::setDebounce(v);
    drawSetting();
  }
  if (millis() - lastDraw >= 100) {
    lastDraw = millis();
    drawLive();
  }
}
