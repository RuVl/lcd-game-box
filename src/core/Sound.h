// Пассивный зуммер: мелодии из PROGMEM играют фоном, loop() не блокируется.
#pragma once

#include <Arduino.h>

struct Note {
  uint16_t freq;  // Гц; 1 — пауза
  uint16_t ms;    // 0 — конец мелодии
};

namespace Sound {

void begin();
void update();  // вызывать каждый проход loop()

void play(const Note *progmemMelody);  // прерывает текущую мелодию
void stop();
bool playing();

}  // namespace Sound
