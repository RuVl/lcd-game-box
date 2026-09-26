#include "core/Launcher.h"

#include "core/Display.h"
#include "core/GameRegistry.h"
#include "core/Input.h"
#include "core/Sound.h"

namespace Launcher {

namespace {
const Note SND_MOVE[] PROGMEM = {{880, 30}, {0, 0}};
const Note SND_SELECT[] PROGMEM = {{660, 60}, {990, 90}, {0, 0}};

uint8_t selected;
Game *game;  // nullptr — показываем меню

void drawSelection() {
  char row[Display::COLS + 1];
  memset(row, ' ', Display::COLS);
  row[Display::COLS] = '\0';
  if (GAME_COUNT > 1) {
    row[0] = '<';
    row[Display::COLS - 1] = '>';
  }
  const char *title = GAMES[selected].title;
  uint8_t len = strlen(title);
  if (len > Display::COLS - 2) len = Display::COLS - 2;
  memcpy(row + (Display::COLS - len) / 2, title, len);
  Display::printRow(1, row);
}

void showMenu() {
  Display::clear();
  Display::printCentered(0, "LCD GAME BOX");
  drawSelection();
  Input::suppressUntilRelease();
}

void startSelected() {
  Sound::play(SND_SELECT);
  Input::suppressUntilRelease();
  Display::clear();
  game = GAMES[selected].create(gameArena);
  game->begin();
}
}  // namespace

void begin() { showMenu(); }

void update() {
  if (game) {
    game->update();
    if (game->exitRequested()) {
      game->~Game();
      game = nullptr;
      Sound::stop();
      showMenu();
    }
    return;
  }

  if (GAME_COUNT > 1 && Input::repeat(Input::LEFT | Input::RIGHT)) {
    selected = Input::held(Input::LEFT) ? (selected + GAME_COUNT - 1) % GAME_COUNT
                                        : (selected + 1) % GAME_COUNT;
    Sound::play(SND_MOVE);
    drawSelection();
  }
  if (Input::pressed(Input::CLICK)) startSelected();
}

}  // namespace Launcher
