#include "games/dino/DinoGame.h"

#include "core/Config.h"
#include "core/Display.h"
#include "core/Input.h"
#include "core/Sound.h"
#include "games/dino/DinoData.h"

using namespace dino;

namespace {
constexpr uint8_t HISCORE_SLOT = 3;
constexpr uint16_t TICK_MS = 25;

constexpr uint8_t CELL_W = 6;                         // 5 видимых точек + промежуток
constexpr int16_t VIEW_W = Display::COLS * CELL_W;    // ширина поля в точках
constexpr int16_t DINO_X = 1 * CELL_W;                // динозавр во второй колонке
constexpr uint8_t SCORE_COL = Display::COLS - 5;      // 5 цифр счёта справа сверху

// Скорость в 1/256 точки за шаг: 192 — 30 точек/с (5 клеток/с), 480 — 75 точек/с.
constexpr uint16_t SPEED_START = 192;
constexpr uint16_t SPEED_MAX = 480;
constexpr uint8_t SCORE_PER_SPEED = 8;  // +1 к скорости за каждые 8 очков
constexpr uint8_t DOTS_PER_POINT = 3;

// Просвет от конца одной группы до начала следующей: не меньше MIN_GAP (бюджет CGRAM, см.
// DinoGame.h) и не меньше пути за прыжок с запасом на реакцию — после приземления успеваешь.
constexpr int16_t MIN_GAP = 36;
constexpr uint8_t GAP_TICKS = JUMP_LEN + 8;
}  // namespace

// ---------- Отрисовка ----------

// Строки клетки в собираемом кадре; nullptr — символы кончились (при правильных
// расстояниях не бывает).
uint8_t *DinoGame::cellRows(uint8_t pos) {
  for (uint8_t i = 0; i < cellCount; i++)
    if (cells[i].pos == pos) return cells[i].bits;
  if (cellCount == MAX_CELLS) return nullptr;
  Cell &c = cells[cellCount++];
  c.pos = pos;
  memset(c.bits, 0, sizeof(c.bits));
  return c.bits;
}

// Наложить 5 точек строки на клетку; true — какая-то точка легла на уже нарисованную.
bool DinoGame::plot(int8_t col, uint8_t y, uint8_t bits) {
  if (!bits || col < 0 || col >= Display::COLS) return false;
  uint8_t *rows = cellRows((y >> 3) * Display::COLS + col);
  if (!rows) return false;
  uint8_t &r = rows[y & 7];
  bool hit = r & bits;
  r |= bits;
  return hit;
}

// Спрайт со сдвигом sub точек ложится на две соседние клетки. В 12-битном окне
// (бит 11 — левая точка) точки 0…4 — клетка col, 5 — промежуток, 6…10 — клетка col + 1.
bool DinoGame::drawSprite(uint8_t spr, int16_t x, uint8_t y) {
  uint8_t xs = x + CELL_W;  // x ≥ -4: делим неотрицательное
  int8_t col = xs / CELL_W - 1;
  uint8_t sub = xs % CELL_W;
  bool hit = false;
  for (uint8_t r = 0; r < 8; r++, y++) {
    uint16_t v = ((uint16_t)pgm_read_byte(&SPRITES[spr][r]) << 7) >> sub;
    hit |= plot(col, y, (v >> 7) & 0x1F);
    hit |= plot(col + 1, y, (v >> 1) & 0x1F);
  }
  return hit;
}

// Динозавр рисуется первым, поэтому пересечение при наложении препятствия — столкновение.
bool DinoGame::render(bool dinoVisible) {
  cellCount = 0;
  if (dinoVisible) {
    bool frame = (tickCount >> 2) & 1;
    if (jumpT)
      drawSprite(S_JUMP, DINO_X, DINO_TOP - pgm_read_byte(&JUMP[jumpT - 1]));
    else if (ducking)
      drawSprite(frame ? S_DUCK_B : S_DUCK_A, DINO_X, DUCK_TOP);
    else
      drawSprite(frame ? S_RUN_B : S_RUN_A, DINO_X, DINO_TOP);
  }
  bool hit = false;
  bool wings = (tickCount >> 3) & 1;
  for (uint8_t i = 0; i < obsCount; i++) {
    ObsDef d;
    memcpy_P(&d, &OBSTACLES[obs[i].type], sizeof(d));
    uint8_t spr = d.spr + (d.spr == S_BIRD_A && wings ? 1 : 0);
    int16_t x = obs[i].x;
    for (uint8_t k = 0; k < d.n; k++, x += d.pitch)  // группа кактусов — несколько спрайтов
      if (x > -5 && x < VIEW_W && drawSprite(spr, x, d.top)) hit = true;
  }
  flushCells();
  return hit;
}

// Вывести кадр: i-я занятая клетка показывается символом CGRAM i. Порядок клеток от кадра
// к кадру почти не меняется (сначала динозавр, потом препятствия по порядку), поэтому
// символ перезаписывается, только когда его точки изменились. Пока идёт вывод, клетка
// может пару миллисекунд показывать чужой символ — на медленном ЖК это не заметно.
void DinoGame::flushCells() {
  constexpr uint8_t N = Display::COLS * Display::ROWS;
  uint8_t scr[N];  // символ каждой клетки: код CGRAM (< 8) или обычный
  memset(scr, ' ', N);
  if (!(flashTicks & 4)) {  // после очередной сотни счёт мигает
    uint32_t v = score;
    for (uint8_t p = Display::COLS - 1; p >= SCORE_COL; p--) {
      scr[p] = '0' + v % 10;
      v /= 10;
    }
  }
  for (uint8_t i = 0; i < cellCount; i++) {
    scr[cells[i].pos] = i;
    if (memcmp(slotBits[i], cells[i].bits, 8) != 0) {
      memcpy(slotBits[i], cells[i].bits, 8);
      Display::lcd.createChar(i, slotBits[i]);
    }
  }
  for (uint8_t p = 0; p < N; p++) Display::put(p % Display::COLS, p / Display::COLS, scr[p]);
}

// ---------- Игра ----------

void DinoGame::spawn() {
  uint8_t kinds = 1;  // открытые по счёту виды — начало таблицы OBSTACLES
  while (kinds < O_COUNT && score >= 25u * pgm_read_byte(&OBSTACLES[kinds].unlock)) kinds++;
  uint8_t type = random(kinds);
  if (obsCount < MAX_OBS) obs[obsCount++] = {(int16_t)(VIEW_W + spawnIn), type};
  int16_t gap = (speed * GAP_TICKS) >> 8;
  if (gap < MIN_GAP) gap = MIN_GAP;
  spawnIn += pgm_read_byte(&OBSTACLES[type].w) + gap + random(gap);  // просвет — от 1 до 2 минимальных
}

// Один шаг игры; true — столкновение.
bool DinoGame::gameTick() {
  // ↓, зажатая ещё до прыжка, быстрого падения не включает — иначе прыжок из приседа
  // обрывался на первом же шаге и динозавр дёргался между землёй и воздухом.
  bool down = Input::held(Input::DOWN);
  if (!down) downLocked = false;
  if (jumpT) {
    // ↓, нажатая в прыжке: сразу на нисходящую ветку и вдвое быстрее
    bool fall = down && !downLocked;
    if (fall && jumpT <= JUMP_LEN / 2) jumpT = JUMP_LEN + 1 - jumpT;
    jumpT += fall ? 2 : 1;
    if (jumpT > JUMP_LEN) jumpT = 0;
  }
  // Приземлился — в том же шаге решаем, что дальше: без лишнего кадра между прыжками.
  if (jumpT == 0) {
    ducking = down;
    if (jumpLatched || Input::held(Input::UP | Input::CLICK)) {  // прыжок и из приседа
      jumpT = 1;
      ducking = false;
      downLocked = down;
      Sound::play(SND_JUMP);
    }
  }
  jumpLatched = false;

  scrollAcc += speed;
  uint8_t move = scrollAcc >> 8;
  scrollAcc &= 0xFF;
  for (uint8_t i = 0; i < obsCount; i++) obs[i].x -= move;
  if (obsCount && obs[0].x + pgm_read_byte(&OBSTACLES[obs[0].type].w) <= 0) {  // ушло за левый край; порядок сохраняем — символы не прыгают
    obsCount--;
    memmove(obs, obs + 1, obsCount * sizeof(Obstacle));
  }
  spawnIn -= move;
  if (spawnIn <= 0) spawn();

  // Очки — за пройденные точки; каждые 8 очков скорость растёт, каждые 100 — сигнал.
  for (dotAcc += move; dotAcc >= DOTS_PER_POINT; dotAcc -= DOTS_PER_POINT) {
    if (score == 99999) break;
    score++;
    if (!((uint8_t)score % SCORE_PER_SPEED) && speed < SPEED_MAX) speed++;
    if (++hundred == 100) {
      hundred = 0;
      flashTicks = 32;
      Sound::play(SND_POINT);
    }
  }
  if (flashTicks) flashTicks--;

  return render();
}

// ---------- Конечный автомат ----------

void DinoGame::begin() { enter(State::Title); }

void DinoGame::enter(State s) {
  state = s;
  phase.reset();
}

void DinoGame::update() {
  switch (state) {
    case State::Title: updateTitle(); break;
    case State::Playing: updatePlaying(); break;
    case State::Over: updateOver(); break;
  }
}

void DinoGame::startPlaying() {
  obsCount = 0;
  spawnIn = VIEW_W / 2;  // первое препятствие — через полтора экрана
  speed = SPEED_START;
  scrollAcc = 0;
  dotAcc = 0;
  score = 0;
  hundred = 0;
  flashTicks = 0;
  jumpT = 0;
  ducking = false;
  downLocked = false;
  jumpLatched = false;
  tickCount = 0;
  Display::clear();
  memset(slotBits, 0xFF, sizeof(slotBits));  // CGRAM неизвестно что: перезаписать всё
  render();
  nextTick = millis();
  enter(State::Playing);
}

// Нажатие стика — старт, удержание — выход в меню устройства.
void DinoGame::updateTitle() {
  if (phase.step == 0) {
    Display::loadGlyph(0, SPRITES[S_RUN_A]);
    Display::loadGlyph(1, SPRITES[S_CACTUS_L]);
    title.begin("DINO RUN", HISCORE_SLOT, "UP:JUMP DN:DUCK");
    Display::put(2, 0, 0);
    Display::put(13, 0, 1);
    phase.next();
  }
  switch (title.update()) {
    case TitleScreen::START: startPlaying(); break;
    case TitleScreen::EXIT: requestExit(); break;
    default: break;
  }
}

void DinoGame::updatePlaying() {
  if (Input::anyPressed(Input::UP | Input::CLICK)) jumpLatched = true;
  if ((int32_t)(millis() - nextTick) < 0) return;
  nextTick += TICK_MS;
  tickCount++;
  if (gameTick()) {
    Sound::play(SND_CRASH);
    enter(State::Over);
  }
}

// Столкновение: поле замирает, динозавр мигает; потом счёт, проверка рекорда
// и на титульный экран — по нажатию или через несколько секунд.
void DinoGame::updateOver() {
  constexpr uint8_t BLINKS = 6;
  if (phase.step < BLINKS) {
    if (phase.elapsed() < 180) return;
    render(phase.step % 2 == 1);
    phase.next();
  } else if (phase.step == BLINKS) {
    if (phase.elapsed() < 180) return;
    result.begin("GAME OVER", score, HISCORE_SLOT);
    phase.next();
  } else {
    bool hadRecord = result.newRecord();
    if (result.update()) enter(State::Title);
    else if (!hadRecord && result.newRecord()) Sound::play(SND_RECORD);
  }
}
