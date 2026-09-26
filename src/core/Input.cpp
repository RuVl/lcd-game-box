#include "core/Input.h"

#include <EncButton.h>

#include "core/Config.h"
#include "core/Storage.h"

namespace Input {

namespace {
constexpr uint8_t COUNT = 5;  // LEFT, RIGHT, UP, DOWN, CLICK — в порядке битов маски

VirtButton buttons[COUNT];
bool axisState[4];  // состояние направлений с гистерезисом
int lastX, lastY, midX, midY;
int offX, offY;          // наклон от центра: вправо и вверх — положительные
int onDelta, offDelta;   // пороги из чувствительности
uint32_t lastSample;
uint8_t heldMask, prevHeldMask;
bool suppressed = true;  // кнопка, зажатая при включении, не считается нажатием
// События (нажал/отпустил/автоповтор) не выдаются и на том проходе, где блокировка снялась:
// иначе отпускание кнопки, зажатой с прошлого экрана, выглядело бы как клик.
bool eventsBlocked = true;
uint32_t clickStart, clickDuration;

// ---- чтение осей ----
#if defined(ESP8266)
// Одна ось на A0, вторая «отключена» выводом MUX в LOW.
int readAxis(uint8_t enablePin, uint8_t disablePin) {
  pinMode(disablePin, OUTPUT);
  digitalWrite(disablePin, LOW);
  pinMode(enablePin, INPUT);
  delayMicroseconds(30);  // перезаряд входа АЦП через 1 кОм и диод
  return analogRead(config::PIN_JOY_ADC);
}
int readX() { return readAxis(config::PIN_MUX_X, config::PIN_MUX_Y); }
int readY() { return readAxis(config::PIN_MUX_Y, config::PIN_MUX_X); }
#else
int readX() { return analogRead(config::PIN_JOY_X); }
int readY() { return analogRead(config::PIN_JOY_Y); }
#endif

// Направление с гистерезисом: включается за onDelta от центра, выключается ближе offDelta.
bool hysteresis(bool on, int offset) { return on ? offset > offDelta : offset > onDelta; }

void sampleAxes() {
  lastX = readX();
  lastY = readY();
  int dxv = offX = config::JOY_INVERT_X ? midX - lastX : lastX - midX;
  int dyv = offY = config::JOY_INVERT_Y ? lastY - midY : midY - lastY;  // «вверх» — положительное
  axisState[0] = hysteresis(axisState[0], -dxv);  // LEFT
  axisState[1] = hysteresis(axisState[1], dxv);   // RIGHT
  axisState[2] = hysteresis(axisState[2], dyv);   // UP
  axisState[3] = hysteresis(axisState[3], -dyv);  // DOWN
}

bool anyEvent(uint8_t mask, bool (VirtButton::*event)()) {
  if (eventsBlocked) return false;
  for (uint8_t i = 0; i < COUNT; i++)
    if ((mask & (1 << i)) && (buttons[i].*event)()) return true;
  return false;
}
}  // namespace

void begin() {
  pinMode(config::PIN_JOY_SW, INPUT_PULLUP);
  // Центр стика — среднее нескольких чтений при включении (стик в этот момент не трогают).
  long sx = 0, sy = 0;
  for (uint8_t i = 0; i < 8; i++) {
    sx += readX();
    sy += readY();
    delay(2);
  }
  midX = sx / 8;
  midY = sy / 8;
  setSensitivity(Storage::loadSetting(Storage::JOY_SENSITIVITY, config::DEFAULT_SENSITIVITY));
  setDebounce(Storage::loadSetting(Storage::JOY_DEBOUNCE, config::DEFAULT_DEBOUNCE));
  for (VirtButton &b : buttons) {
    b.setHoldTimeout(config::REPEAT_DELAY_MS);
    b.setStepTimeout(config::REPEAT_STEP_MS);
  }
}

void update() {
  if (millis() - lastSample >= config::JOY_SAMPLE_MS) {
    lastSample = millis();
    sampleAxes();
  }
  bool click = digitalRead(config::PIN_JOY_SW) == LOW;

  prevHeldMask = heldMask;
  heldMask = 0;
  for (uint8_t i = 0; i < COUNT; i++) {
    bool raw = i < 4 ? axisState[i] : click;
    buttons[i].tick(raw);
    if (buttons[i].pressing()) heldMask |= 1 << i;
  }

  clickDuration = 0;
  eventsBlocked = suppressed;
  if (suppressed) {
    if (heldMask == 0) suppressed = false;
    return;
  }
  if (buttons[4].press()) clickStart = millis();
  if (buttons[4].release()) clickDuration = millis() - clickStart;
}

bool held(uint8_t mask) { return !suppressed && (heldMask & mask); }
bool pressed(uint8_t mask) { return anyEvent(mask, &VirtButton::press); }
bool released(uint8_t mask) { return anyEvent(mask, &VirtButton::release); }
bool anyPressed(uint8_t mask) { return !suppressed && (heldMask & mask) && !(prevHeldMask & mask); }
bool repeat(uint8_t mask) { return pressed(mask) || anyEvent(mask, &VirtButton::step); }

int8_t dx() {
  if (held(LEFT)) return -1;
  if (held(RIGHT)) return 1;
  return 0;
}

uint32_t clickHeldMs() { return held(CLICK) ? millis() - clickStart : 0; }
uint32_t lastClickMs() { return clickDuration; }

uint8_t direction() {
  uint8_t h = heldMask & (LEFT | RIGHT | UP | DOWN);
  if (!h || suppressed) return 0;
  bool xAxis = abs(offX) >= abs(offY);
  uint8_t pick = h & (xAxis ? (LEFT | RIGHT) : (UP | DOWN));
  return pick ? pick : h & (xAxis ? (UP | DOWN) : (LEFT | RIGHT));
}

void setSensitivity(uint8_t level) {
  level = constrain(level, 1, 10);
  onDelta = 340 - level * 26;       // 1 → 314, 5 → 210, 10 → 80
  offDelta = onDelta * 3 / 5;
}

void setDebounce(uint8_t level) {
  level = constrain(level, 1, 10);
  for (VirtButton &b : buttons) b.setDebTimeout(level * 10);
}

void suppressUntilRelease() {
  suppressed = true;
  eventsBlocked = true;
}

int rawX() { return lastX; }
int rawY() { return lastY; }
int centerX() { return midX; }
int centerY() { return midY; }

}  // namespace Input
