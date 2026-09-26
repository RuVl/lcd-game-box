#include "core/GameScreens.h"

#include "core/Config.h"
#include "core/Display.h"
#include "core/Input.h"
#include "core/Storage.h"
#include "core/Text.h"

namespace {
constexpr uint16_t LINE_MS = 900;
constexpr uint16_t RECORD_DELAY_MS = 1500;
constexpr uint16_t RESULT_MS = 3500;
}  // namespace

// ---------- TitleScreen ----------

void TitleScreen::begin(const char *title, uint8_t hiScoreSlot, const char *extra) {
  extra_ = extra;
  line_ = 0;
  Input::suppressUntilRelease();
  Display::clear();
  Display::printCentered(0, title);
  Text::num(Text::str(top_, "TOP "), Storage::loadHiScore(hiScoreSlot), 6);
  showLine();
}

void TitleScreen::showLine() {
  const char *text;
  switch (line_) {
    case 0: text = "PRESS BUTTON"; break;
    case 1: text = extra_ ? extra_ : top_; break;
    case 2: text = extra_ ? top_ : "HOLD: MENU"; break;
    default: text = "HOLD: MENU"; break;
  }
  Display::printCentered(1, text);
  phase_.reset();
}

TitleScreen::Action TitleScreen::update() {
  if (phase_.elapsed() >= LINE_MS) {
    line_ = (line_ + 1) % (extra_ ? 4 : 3);
    showLine();
  }
  if (Input::clickHeldMs() >= config::HOLD_TO_EXIT_MS) return EXIT;
  if (Input::released(Input::CLICK) && Input::lastClickMs() < config::HOLD_TO_EXIT_MS) {
    randomSeed(micros());  // момент нажатия случаен
    return START;
  }
  return NONE;
}

void TitleScreen::flash(const char *text) {
  Display::printCentered(1, text);
  phase_.reset();  // надпись висит полный интервал
}

void TitleScreen::setExtra(const char *text) {
  extra_ = text;
  line_ = 1;
  showLine();
}

// ---------- ResultScreen ----------

void ResultScreen::begin(const char *title, uint32_t score, uint8_t hiScoreSlot, const char *detail,
                         bool saveRecord) {
  score_ = score;
  slot_ = hiScoreSlot;
  saveRecord_ = saveRecord;
  newRecord_ = false;
  Input::suppressUntilRelease();  // кнопку могли держать в игре (прыжок, огонь, нитро)
  Display::clear();
  Display::printCentered(0, title);
  char buf[17];
  if (!detail) {
    Text::num(Text::str(buf, "SCORE "), score, 6);
    detail = buf;
  }
  Display::printCentered(1, detail);
  phase_.reset();
}

bool ResultScreen::update() {
  if (phase_.step == 0) {
    if (phase_.elapsed() < RECORD_DELAY_MS) return false;
    if (saveRecord_ && score_ > Storage::loadHiScore(slot_)) {
      Storage::saveHiScore(slot_, score_);
      Display::printCentered(0, "NEW RECORD!");
      newRecord_ = true;
    }
    phase_.next();
    return false;
  }
  return phase_.elapsed() >= RESULT_MS || Input::pressed(Input::CLICK);
}
