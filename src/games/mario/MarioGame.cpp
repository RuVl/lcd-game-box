#include "games/mario/MarioGame.h"

#include "core/Config.h"
#include "core/Display.h"
#include "core/Input.h"
#include "core/Sound.h"
#include "core/Text.h"
#include "games/mario/MarioLevels.h"
#include "games/mario/MarioSounds.h"
#include "games/mario/MarioSprites.h"

using namespace mario;

namespace {
constexpr uint8_t NO_SLOT = 0xFF;
constexpr uint8_t MARIO_GLYPHS_UNKNOWN = 0xFF;

bool isPitTile(char c) { return c == ' ' || c == 'f'; }
bool isPipe(char c) { return c == 'P' || c == 'T'; }
bool isBlock(char c) { return c == '?' || c == 'M' || c == '#' || c == 'c' || c == 'm' || c == 'u'; }
bool isWalkable(char c) { return c == '_' || c == '='; }  // куда могут ходить враги

// Финальный экран
constexpr uint8_t ENDING_WALK_TO = 11;
constexpr uint8_t SLOT_PRINCESS = 1;
constexpr uint8_t SLOT_HEART = 2;
}  // namespace

// ---------- Спрайты и отрисовка ----------

// Символов у HD44780 всего 8. Символ 0 — Марио: все его позы по очереди загружаются туда.
// Остальные 7 раздаются при старте уровня только тем спрайтам, что в нём встречаются.
void MarioGame::loadLevelSprites(uint32_t mask) {
  uint8_t slot = 1;
  for (uint8_t s = 0; s < S_COUNT; s++) {
    slotOf[s] = NO_SLOT;
    if (!(mask & (1UL << s))) continue;
    if (slot > 7) continue;  // не влезло — уровни проверены скриптом, сюда не попадаем
    slotOf[s] = slot;
    Display::loadGlyph(slot, (const uint8_t *)pgm_read_ptr(&SPRITE_A[s]));
    slot++;
  }
}

void MarioGame::animateSprites() {
  anim = !anim;
  for (uint8_t s = 0; s < S_COUNT; s++) {
    const uint8_t *b = (const uint8_t *)pgm_read_ptr(&SPRITE_B[s]);
    if (!b || slotOf[s] == NO_SLOT) continue;
    Display::loadGlyph(slotOf[s], anim ? b : (const uint8_t *)pgm_read_ptr(&SPRITE_A[s]));
  }
}

char MarioGame::glyph(Spr s) const { return slotOf[s] == NO_SLOT ? ' ' : slotOf[s]; }

void MarioGame::setMarioGlyph(MarioPose pose, bool left, bool isBig) {
  uint8_t want = pose | (left ? 4 : 0) | (isBig ? 8 : 0);
  if (want == marioGlyphState) return;
  const uint8_t *spr;
  if (isBig)
    spr = pose == POSE_RUN ? SPR_BIG_RUN : pose == POSE_AIR ? SPR_BIG_AIR : SPR_BIG;
  else
    spr = pose == POSE_RUN ? SPR_MARIO_RUN : pose == POSE_AIR ? SPR_MARIO_AIR : SPR_MARIO;
  Display::loadGlyph(0, spr, left);
  marioGlyphState = want;
}

void MarioGame::render(bool marioVisible) {
  // Во время неуязвимости Марио мигает.
  if (invuln && (tickCount & 1)) marioVisible = false;
  for (uint8_t col = 0; col < Display::COLS; col++) {
    uint8_t x = camX + col;
    char t;
    switch (topRow[x]) {
      case 'o':
      case 'c': t = glyph(S_COIN); break;
      case '?':
      case 'M': t = glyph(S_QBLOCK); break;
      case 'm': t = glyph(S_MUSHROOM); break;
      case '#': t = glyph(S_BRICK); break;
      case 'u': t = glyph(S_USED); break;
      default: t = ' '; break;
    }
    char b;
    switch (bottomRow[x]) {
      case '_':
      case '=': b = glyph(S_GROUND); break;
      case 'P':
      case 'T': b = glyph(S_PIPE); break;
      case 'S': b = glyph(S_SPRING); break;
      case '|': b = '|'; break;
      case 'A': b = glyph(S_AXE); break;
      default: b = level.castle ? glyph(S_LAVA) : ' '; break;
    }
    if (x == goalX && bottomRow[x] == '|') t = 'F';  // флаг на верхушке
    if (liftAt(x)) b = glyph(S_LIFT);
    if (plantUp(x)) t = glyph(S_PLANT);
    int8_t pr = podRow(x);
    if (pr == 0) t = glyph(S_FIRE);
    if (pr == 1) b = glyph(S_FIRE);
    for (uint8_t i = 0; i < MAX_FLAMES; i++)
      if (flames[i].active && flames[i].x == x) t = glyph(S_FIRE);
    int8_t e = enemyAt(x);
    if (e >= 0) {
      uint8_t type = enemies[e].type;
      b = glyph(type == E_GOOMBA ? S_GOOMBA : type == E_KOOPA ? S_KOOPA : S_SHELL);
    }
    if (bowser.alive && bowser.x == x) {
      if (bowser.y == 0)
        t = glyph(S_BOWSER);
      else
        b = glyph(S_BOWSER);
    }

    if (marioVisible && x == marioX) {
      bool inAir = marioY == 0 || !solidAt(x);
      setMarioGlyph(inAir ? POSE_AIR : (runFrame ? POSE_RUN : POSE_STAND), faceLeft, big);
      if (marioY == 0)
        t = 0;
      else
        b = 0;
    }
    Display::put(col, 0, t);
    Display::put(col, 1, b);
  }
}

// ---------- Уровень ----------

// Какие спрайты нужны уровню. Те же правила — в tools/check_levels.py.
uint32_t MarioGame::usedSprites() const {
  uint32_t m = 1UL << S_GROUND;
  bool pit = false;
  for (uint8_t i = 0; i < levelLen; i++) {
    switch (topRow[i]) {
      case 'o': m |= 1UL << S_COIN; break;
      case '?': m |= (1UL << S_QBLOCK) | (1UL << S_USED) | (1UL << S_COIN); break;
      case 'M': m |= (1UL << S_QBLOCK) | (1UL << S_USED) | (1UL << S_MUSHROOM); break;
      case '#': m |= 1UL << S_BRICK; break;
    }
    switch (pgm_read_byte(level.bottom + i)) {
      case 'P': m |= 1UL << S_PIPE; break;
      case 'T': m |= (1UL << S_PIPE) | (1UL << S_PLANT); break;
      case 'G': m |= 1UL << S_GOOMBA; break;
      case 'K': m |= (1UL << S_KOOPA) | (1UL << S_SHELL); break;
      case 'L': m |= 1UL << S_LIFT; break;
      case 'S': m |= 1UL << S_SPRING; break;
      case 'f': m |= 1UL << S_FIRE; pit = true; break;
      case 'B': m |= 1UL << S_BOWSER; if (level.bowserFire) m |= 1UL << S_FIRE; break;
      case 'A': m |= 1UL << S_AXE; break;
      case ' ': pit = true; break;
    }
  }
  if (level.castle && pit) m |= 1UL << S_LAVA;
  return m;
}

void MarioGame::resetLevel() {
  memcpy_P(&level, &LEVELS[levelIdx], sizeof(LevelDef));
  levelLen = strlen_P(level.bottom);
  enemyCount = 0;
  liftCount = 0;
  bowser.alive = false;
  memset(flames, 0, sizeof(flames));
  for (uint8_t i = 0; i < levelLen; i++) {
    topRow[i] = pgm_read_byte(level.top + i);
    char c = pgm_read_byte(level.bottom + i);
    if (c == 'G' || c == 'K') {
      if (enemyCount < MAX_ENEMIES) enemies[enemyCount++] = {i, -1, c == 'G' ? E_GOOMBA : E_KOOPA};
      c = '_';
    } else if (c == 'B') {
      bowser = {i, 1, -1, 0, true};
      c = '=';
    } else if (c == 'L') {
      c = ' ';
    }
    if (c == '|' || c == 'A') goalX = i;
    bottomRow[i] = c;
  }
  // Платформы: подряд идущие 'L' — одна платформа, её путь — вся яма вокруг.
  for (uint8_t i = 0; i < levelLen; i++) {
    if (pgm_read_byte(level.bottom + i) != 'L') continue;
    uint8_t j = i;
    while (j < levelLen && pgm_read_byte(level.bottom + j) == 'L') j++;
    uint8_t lo = i, hi = j - 1;
    while (lo > 0 && bottomRow[lo - 1] == ' ') lo--;
    while (hi + 1 < levelLen && bottomRow[hi + 1] == ' ') hi++;
    if (liftCount < MAX_LIFTS) lifts[liftCount++] = {lo, hi, (uint8_t)(j - i), (uint8_t)(i - lo)};
    i = j - 1;
  }
  marioX = 1;
  marioY = 1;
  airTicks = 0;
  faceLeft = false;
  runFrame = false;
  invuln = 0;
  camX = 0;
  tickCount = 0;
  popTicks = 0;
  marioGlyphState = MARIO_GLYPHS_UNKNOWN;
  setMarioGlyph(POSE_STAND, false, big);
  loadLevelSprites(usedSprites());
  Display::clear();
}

uint8_t MarioGame::liftLeft(const Lift &l, uint32_t t) const {
  uint8_t m = l.hi - l.lo + 1 - l.w + 1;  // сколько положений у платформы
  if (m <= 1) return l.lo;
  uint16_t cyc = 2 * (m - 1);
  uint16_t k = (t / LIFT_EVERY + l.o0) % cyc;
  return l.lo + (k < m ? k : cyc - k);
}

bool MarioGame::liftAt(uint8_t x) const {
  for (uint8_t i = 0; i < liftCount; i++) {
    uint8_t a = liftLeft(lifts[i], tickCount);
    if (a <= x && x < a + lifts[i].w) return true;
  }
  return false;
}

bool MarioGame::solidAt(uint8_t x) const { return !isPitTile(bottomRow[x]) || liftAt(x); }

bool MarioGame::plantUp(uint8_t x) const {
  return bottomRow[x] == 'T' && (tickCount + x * 3UL) % PLANT_PERIOD < PLANT_UP;
}

// Огненный шар из лавы: в каком ряду он сейчас (-1 — спрятан в лаве).
int8_t MarioGame::podRow(uint8_t x) const {
  if (bottomRow[x] != 'f') return -1;
  uint8_t ph = (tickCount + x * 5UL) % POD_PERIOD;
  if (ph == 0 || ph == 4) return 1;
  if (ph <= 3) return 0;
  return -1;
}

int8_t MarioGame::enemyAt(uint8_t x, int8_t except) const {
  for (uint8_t i = 0; i < enemyCount; i++)
    if (i != except && enemies[i].type != E_NONE && enemies[i].x == x) return i;
  return -1;
}

bool MarioGame::onPipe() const { return marioY == 0 && isPipe(bottomRow[marioX]); }

// ---------- Марио ----------

void MarioGame::addCoin() {
  coins++;
  score += 200;
  if (coins % COINS_PER_1UP == 0 && lives < MAX_LIVES) {
    lives++;
    Sound::play(SND_1UP);
  } else {
    Sound::play(SND_COIN);
  }
}

void MarioGame::collectCoinAt(uint8_t x) {
  if (marioY == 0 && topRow[x] == 'o') {
    topRow[x] = ' ';
    addCoin();
  }
}

void MarioGame::finishPop() {
  if (popTicks == 0) return;
  topRow[popX] = 'u';
  popTicks = 0;
}

// Удар по блоку снизу.
void MarioGame::hitBlock(uint8_t x) {
  char c = topRow[x];
  if (c != '?' && c != 'M') {
    Sound::play(SND_BUMP);
    return;
  }
  finishPop();
  popX = x;
  popTicks = POP_TICKS;
  if (c == '?') {
    topRow[x] = 'c';
    addCoin();
  } else {
    topRow[x] = 'm';
    if (big) {
      score += 1000;
      Sound::play(SND_COIN);
    } else {
      big = true;
      Sound::play(SND_POWERUP);
    }
  }
}

// Марио задели. true — погиб; с грибом он только уменьшается и ненадолго становится неуязвим.
bool MarioGame::hurt() {
  if (invuln) return false;
  if (big) {
    big = false;
    invuln = INVULN_TICKS;
    Sound::play(SND_HURT);
    return false;
  }
  return true;
}

// ---------- Враги ----------

void MarioGame::killEnemy(int8_t e) {
  enemies[e].type = E_NONE;
  score += 100;
}

// Пнуть стоящий панцирь. false — упёрся в трубу, пнуть нельзя.
bool MarioGame::kickShell(int8_t e, int8_t dir) {
  Enemy &en = enemies[e];
  int16_t nx = en.x + dir;
  if (nx < 0 || nx >= levelLen || isPipe(bottomRow[nx])) return false;
  int8_t other = enemyAt(nx, e);
  if (other >= 0) killEnemy(other);
  en.x = nx;
  en.dir = dir;
  en.type = solidAt(nx) ? E_SHELL_MOVING : E_NONE;  // пнули в яму — упал
  Sound::play(SND_KICK);
  return true;
}

// Прыжок сверху на врага.
void MarioGame::stompEnemy(int8_t e) {
  Enemy &en = enemies[e];
  switch (en.type) {
    case E_GOOMBA:
      killEnemy(e);
      Sound::play(SND_STOMP);
      break;
    case E_KOOPA:
    case E_SHELL_MOVING:
      en.type = E_SHELL;  // купа прячется в панцирь, катящийся панцирь останавливается
      score += 100;
      Sound::play(SND_STOMP);
      break;
    case E_SHELL: {  // прыжок на стоящий панцирь пинает его вперёд
      int8_t dir = faceLeft ? -1 : 1;
      if (!kickShell(e, dir) && !kickShell(e, -dir)) killEnemy(e);
      break;
    }
  }
}

// Катящиеся панцири: каждый шаг, сбивают всех на пути.
void MarioGame::moveShells() {
  for (uint8_t i = 0; i < enemyCount; i++) {
    Enemy &en = enemies[i];
    if (en.type != E_SHELL_MOVING) continue;
    if (en.x < camX) {  // укатился за левый край экрана
      en.type = E_NONE;
      continue;
    }
    int16_t nx = en.x + en.dir;
    if (nx < 0 || nx >= levelLen || isPipe(bottomRow[nx])) {
      en.dir = -en.dir;
      continue;
    }
    int8_t other = enemyAt(nx, i);
    if (other >= 0) {
      killEnemy(other);
      Sound::play(SND_KICK);
    }
    en.x = nx;
    if (!solidAt(nx)) en.type = E_NONE;
  }
}

// Гумбы и купы: ходят по земле и мосту, разворачиваются у ям, труб и других врагов.
void MarioGame::moveWalkers() {
  for (uint8_t i = 0; i < enemyCount; i++) {
    Enemy &en = enemies[i];
    if ((en.type != E_GOOMBA && en.type != E_KOOPA) || en.x > camX + 17) continue;
    int16_t nx = en.x + en.dir;
    bool blocked = nx < 0 || nx >= levelLen || !isWalkable(bottomRow[nx]) || enemyAt(nx) >= 0;
    if (blocked)
      en.dir = -en.dir;
    else
      en.x = nx;
  }
}

// Боузер бродит по мосту, чаще в сторону Марио, иногда подпрыгивает (тогда под ним
// можно пробежать), а настоящий ещё и плюётся огнём по верхнему ряду.
void MarioGame::moveBowser() {
  if (!bowser.alive || bowser.x > camX + 15) return;
  if (bowser.y == 0) {
    if (bowser.air > 0)
      bowser.air--;
    else
      bowser.y = 1;
    return;
  }
  if (level.bowserFire && tickCount % BOWSER_FIRE_EVERY == 0 && bowser.x > 0) {
    for (uint8_t i = 0; i < MAX_FLAMES; i++) {
      if (flames[i].active) continue;
      flames[i] = {(uint8_t)(bowser.x - 1), true};
      Sound::play(SND_BOWSER_FIRE);
      return;
    }
  }
  uint8_t r = random(8);
  if (r == 0) {
    bowser.y = 0;
    bowser.air = 2;
    Sound::play(SND_BOWSER_JUMP);
    return;
  }
  if (r < 4) bowser.dir = marioX < bowser.x ? -1 : 1;
  else if (r == 4) bowser.dir = -bowser.dir;
  int16_t nx = bowser.x + bowser.dir;
  if (nx >= 0 && nx < levelLen && isWalkable(bottomRow[nx]))
    bowser.x = nx;
  else
    bowser.dir = -bowser.dir;
}

void MarioGame::moveFlames() {
  for (uint8_t i = 0; i < MAX_FLAMES; i++) {
    Flame &f = flames[i];
    if (!f.active) continue;
    if (f.x <= camX || isBlock(topRow[f.x - 1]))
      f.active = false;
    else
      f.x--;
  }
}

bool MarioGame::bowserHitsMario() const { return bowser.alive && bowser.x == marioX && bowser.y == marioY; }

bool MarioGame::flameHitsMario() const {
  if (marioY != 0) return false;
  for (uint8_t i = 0; i < MAX_FLAMES; i++)
    if (flames[i].active && flames[i].x == marioX) return true;
  return false;
}

// Опасно ли то, что сейчас в клетке Марио (кроме ям).
bool MarioGame::marioTouchesDanger() const {
  if (marioY == 1) {
    int8_t e = enemyAt(marioX);
    if (e >= 0 && enemies[e].type != E_SHELL) return true;  // стоящий панцирь не опасен
  }
  if (marioY == 0 && plantUp(marioX)) return true;
  if (podRow(marioX) == (int8_t)marioY) return true;
  if (bowserHitsMario() || flameHitsMario()) return true;
  return false;
}

// ---------- Шаг игры ----------
// Порядок: платформы → прыжок → ход → гравитация → враги и ловушки.
// Прыжок раньше хода, иначе нажатие «вправо + прыжок» у края ямы или перед врагом
// сначала делает шаг вниз/во врага, и прыжок уже не срабатывает.
// Те же правила движения повторяет tools/check_levels.py.
MarioGame::Outcome MarioGame::gameTick(int8_t dx, bool jumpPressed) {
  if (popTicks > 0 && --popTicks == 0) topRow[popX] = 'u';
  if (invuln) invuln--;

  // Платформы везут стоящего на них Марио
  if (marioY == 1) {
    for (uint8_t i = 0; i < liftCount; i++) {
      uint8_t a = liftLeft(lifts[i], tickCount - 1), b = liftLeft(lifts[i], tickCount);
      if (a != b && a <= marioX && marioX < a + lifts[i].w) {
        int16_t nx = marioX + (b - a);
        if (nx >= camX) marioX = nx;
        break;
      }
    }
  }

  // Прыжок
  bool jumped = false;
  bool grounded = (marioY == 1 && solidAt(marioX)) || onPipe();
  if (jumpPressed && grounded) {
    if (marioY == 1 && isBlock(topRow[marioX])) {
      hitBlock(marioX);
    } else {
      bool spring = marioY == 1 && bottomRow[marioX] == 'S';
      marioY = 0;
      airTicks = spring ? SPRING_AIR : AIR_TICKS;
      jumped = true;
      Sound::play(spring ? SND_SPRING : SND_JUMP);
      collectCoinAt(marioX);
    }
  }

  // Горизонталь
  runFrame = false;
  if (dx != 0) {
    faceLeft = dx < 0;
    int16_t nx = marioX + dx;
    bool blocked = nx < camX || nx >= levelLen ||
                   (marioY == 1 && isPipe(bottomRow[nx])) ||
                   (marioY == 0 && isBlock(topRow[nx]));
    if (!blocked && marioY == 1) {
      int8_t e = enemyAt(nx);
      if (e >= 0 && enemies[e].type == E_SHELL && !kickShell(e, dx)) blocked = true;
    }
    if (!blocked) {
      marioX = nx;
      runFrame = (tickCount & 1);
      collectCoinAt(marioX);
      if (marioTouchesDanger() && hurt()) return Outcome::Died;
    }
  }

  // Гравитация
  bool landed = false;
  if (marioY == 0 && !jumped) {
    if (onPipe()) {
      airTicks = 0;
    } else if (airTicks > 0) {
      airTicks--;
    } else {
      marioY = 1;
      landed = true;
      int8_t e = enemyAt(marioX);
      if (e >= 0) stompEnemy(e);
    }
  }

  if (marioY == 1 && !solidAt(marioX)) return Outcome::Died;  // упал в яму
  if (marioX == goalX) return Outcome::Cleared;

  // Враги
  moveShells();
  if (tickCount % level.enemyEvery == 0) moveWalkers();
  if (tickCount % BOWSER_EVERY == 0) moveBowser();
  moveFlames();
  if (landed && marioY == 1) {
    // Враг шагнул под Марио в тот же шаг, когда он приземлился, — считаем, что он на него прыгнул.
    int8_t e = enemyAt(marioX);
    if (e >= 0 && (enemies[e].type == E_GOOMBA || enemies[e].type == E_KOOPA)) stompEnemy(e);
  }
  if (marioTouchesDanger() && hurt()) return Outcome::Died;

  // Камера едет только вперёд, как в оригинале
  if (marioX > camX + CAM_LEAD) camX = marioX - CAM_LEAD;
  if (camX > levelLen - Display::COLS) camX = levelLen - Display::COLS;

  return Outcome::Playing;
}

// ---------- Конечный автомат ----------
// update() никогда не ждёт: каждое состояние делится на шаги (Phase), шаг переключается
// по таймеру или по окончании мелодии. Шаг 0 — действие при входе в состояние.

void MarioGame::begin() {
  marioGlyphState = MARIO_GLYPHS_UNKNOWN;
  enter(State::Title);
}

void MarioGame::enter(State s) {
  state = s;
  phase.reset();
}

void MarioGame::startPlaying() {
  resetLevel();
  render();
  jumpLatched = false;
  nextTick = millis();
  enter(State::Playing);
}

// «$07  001234» — монеты и очки.
void MarioGame::formatCoinsScore(char *buf) const {
  Text::num(Text::str(Text::num(Text::str(buf, "$"), coins, 2), "  "), score, 6);
}

void MarioGame::resultScreen(const char *title) {
  Display::clear();
  Display::printCentered(0, title);
  char buf[17];
  formatCoinsScore(buf);
  Display::printCentered(1, buf);
}

void MarioGame::update() {
  switch (state) {
    case State::Title: updateTitle(); break;
    case State::StartJingle: updateStartJingle(); break;
    case State::World: updateWorld(); break;
    case State::Playing: updatePlaying(); break;
    case State::Dying: updateDying(); break;
    case State::BridgeCollapse: updateBridgeCollapse(); break;
    case State::LevelClear: updateLevelClear(); break;
    case State::AnotherCastle: updateAnotherCastle(); break;
    case State::Ending: updateEnding(); break;
    case State::Final: updateFinal(); break;
  }
}

// Нажатие стика — старт (наклоны нужны для чит-кода), удержание — выход в меню устройства.
void MarioGame::updateTitle() {
  if (phase.step == 0) {
    title.begin("SUPER MARIO LCD", HISCORE_SLOT, "SAVE PRINCESS!");
    phase.next();
  }
  switch (title.update()) {
    case TitleScreen::START:
      lives = START_LIVES;
      coins = 0;
      score = 0;
      levelIdx = 0;
      big = false;
      won = false;
      Sound::play(SND_START);
      enter(State::StartJingle);
      break;
    case TitleScreen::EXIT: requestExit(); break;
    default: break;
  }
}

void MarioGame::updateStartJingle() {
  if (!Sound::playing()) enter(State::World);
}

void MarioGame::updateWorld() {
  if (phase.step == 0) {
    LevelDef def;
    memcpy_P(&def, &LEVELS[levelIdx], sizeof(LevelDef));
    marioGlyphState = MARIO_GLYPHS_UNKNOWN;
    setMarioGlyph(POSE_STAND, false, big);
    Display::clear();
    char buf[17];
    Text::num(Text::chr(Text::num(Text::str(buf, "WORLD "), def.world), '-'), def.num);
    Display::printCentered(0, buf);
    Display::lcd.setCursor(1, 1);
    Display::lcd.write(static_cast<uint8_t>(8));  // Марио (CGRAM 0)
    Display::lcd.print(" x");
    Display::lcd.print(lives);
    Display::lcd.setCursor(8, 1);
    Display::lcd.print("$");
    Display::lcd.print(coins);
    phase.next();
  }
  if (phase.elapsed() >= 2000) startPlaying();
}

void MarioGame::updatePlaying() {
  if (Input::anyPressed(Input::UP | Input::CLICK)) jumpLatched = true;
  if ((int32_t)(millis() - nextTick) < 0) return;
  nextTick += TICK_MS;
  tickCount++;

  Outcome o = gameTick(Input::dx(), jumpLatched);
  jumpLatched = false;
  if (tickCount % 2 == 0) animateSprites();
  render();


  if (o == Outcome::Died) {
    big = false;
    invuln = 0;
    Sound::play(SND_DEATH);
    enter(State::Dying);
  } else if (o == Outcome::Cleared) {
    if (level.castle) {
      bridgeCell = goalX - 1;
      enter(State::BridgeCollapse);
    } else {
      Sound::play(SND_CLEAR);
      enter(State::LevelClear);
    }
  }
}

// Мелодия смерти, затем Марио мигает три раза.
void MarioGame::updateDying() {
  if (phase.step == 0) {
    if (!Sound::playing()) phase.next();
  } else if (phase.step <= 6) {
    if (phase.elapsed() >= 150) {
      render(phase.step % 2 == 0);
      phase.next();
    }
  } else {
    lives--;  // уровень начинается заново
    enter(lives > 0 ? State::World : State::Final);
  }
}

// Топор срублен: мост рушится от топора назад, Боузер падает в лаву.
void MarioGame::updateBridgeCollapse() {
  if (phase.step == 0) {
    if (phase.elapsed() < 90) return;
    phase.restartTimer();
    if (bridgeCell >= 0 && bottomRow[bridgeCell] == '=') {
      bottomRow[bridgeCell] = ' ';
      if (bowser.alive && bowser.x == bridgeCell) bowser.y = 1;
      bridgeCell--;
      Sound::play(SND_BRIDGE);
      render();
    } else {
      if (bowser.alive) {
        bowser.alive = false;  // под ним больше нет моста
        score += 5000;
        render();
      }
      phase.next();
    }
  } else if (phase.elapsed() >= 500) {
    Sound::play(SND_CLEAR);
    enter(State::LevelClear);
  }
}

void MarioGame::updateLevelClear() {
  if (phase.step == 0) {
    if (Sound::playing()) return;
    score += 1000;
    resultScreen("COURSE CLEAR!");
    phase.next();
  } else if (phase.elapsed() >= 2500) {
    bool wasCastle = level.castle;
    if (++levelIdx == LEVEL_COUNT) {
      won = true;
      score += 1000UL * lives;  // бонус за оставшиеся жизни
      enter(State::Ending);
    } else {
      enter(wasCastle ? State::AnotherCastle : State::World);
    }
  }
}

// После ложного Боузера: принцесса в другом замке.
void MarioGame::updateAnotherCastle() {
  if (phase.step == 0) {
    Display::clear();
    Display::printCentered(0, "THANK YOU MARIO!");
    Display::printCentered(1, "BUT OUR PRINCESS");
    phase.next();
  } else if (phase.step == 1 && phase.elapsed() >= 2500) {
    Display::clear();
    Display::printCentered(0, "IS IN ANOTHER");
    Display::printCentered(1, "CASTLE!");
    phase.next();
  } else if (phase.step == 2 && phase.elapsed() >= 2500) {
    enter(State::World);
  }
}

// Марио подходит к принцессе, появляется сердечко, потом надпись.
void MarioGame::updateEnding() {
  if (phase.step == 0) {
    Display::clear();
    Display::loadGlyph(SLOT_PRINCESS, SPR_PRINCESS);
    Display::loadGlyph(SLOT_HEART, SPR_HEART);
    Display::lcd.setCursor(13, 1);
    Display::lcd.write(SLOT_PRINCESS);
    phase.step = 1;
    phase.at = millis() - 220;  // первый шаг Марио сразу
  } else if (phase.step <= ENDING_WALK_TO + 1) {  // шаги 1…12: Марио в колонке step-1
    if (phase.elapsed() < 220) return;
    uint8_t x = phase.step - 1;
    if (x > 0) {
      Display::lcd.setCursor(x - 1, 1);
      Display::lcd.write(' ');
    }
    setMarioGlyph((x & 1) ? POSE_RUN : POSE_STAND, false, big);
    Display::lcd.setCursor(x, 1);
    Display::lcd.write(static_cast<uint8_t>(8));
    phase.next();
  } else if (phase.step == ENDING_WALK_TO + 2) {
    if (phase.elapsed() < 220) return;
    setMarioGlyph(POSE_STAND, false, big);
    Display::lcd.setCursor(12, 0);
    Display::lcd.write(SLOT_HEART);
    Sound::play(SND_ENDING);
    phase.next();
  } else if (phase.step == ENDING_WALK_TO + 3) {
    if (Sound::playing()) {
      phase.restartTimer();  // пауза 800 мс отсчитывается от конца мелодии
    } else if (phase.elapsed() >= 800) {
      Display::clear();
      Display::printCentered(0, "THANK YOU MARIO!");
      Display::printCentered(1, "PRINCESS SAVED");
      phase.next();
    }
  } else if (phase.elapsed() >= 3500) {
    enter(State::Final);
  }
}

// Итог игры, проверка рекорда, возврат на титульный экран.
void MarioGame::updateFinal() {
  if (phase.step == 0) {
    char buf[17];
    formatCoinsScore(buf);
    result.begin(won ? "YOU WIN!" : "GAME OVER", score, HISCORE_SLOT, buf);
    phase.next();
  } else if (result.update()) {
    enter(State::Title);
  }
}
