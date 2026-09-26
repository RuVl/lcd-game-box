// Mario: мелодии в PROGMEM. Подключается только из MarioGame.cpp.
#pragma once

#include "core/Sound.h"

namespace mario {

const Note SND_JUMP[] PROGMEM = {{523, 40}, {784, 60}, {0, 0}};
const Note SND_SPRING[] PROGMEM = {{392, 40}, {523, 40}, {784, 40}, {1047, 80}, {0, 0}};
const Note SND_COIN[] PROGMEM = {{988, 70}, {1319, 180}, {0, 0}};
const Note SND_1UP[] PROGMEM = {{1319, 90}, {1568, 90}, {2637, 90}, {2093, 90}, {2349, 90}, {3136, 150}, {0, 0}};
const Note SND_POWERUP[] PROGMEM = {{523, 60}, {659, 60}, {784, 60}, {1047, 60}, {1319, 60}, {1568, 120}, {0, 0}};
const Note SND_HURT[] PROGMEM = {{880, 60}, {659, 60}, {440, 60}, {330, 120}, {0, 0}};
const Note SND_STOMP[] PROGMEM = {{220, 50}, {330, 60}, {0, 0}};
const Note SND_KICK[] PROGMEM = {{660, 30}, {990, 50}, {0, 0}};
const Note SND_BUMP[] PROGMEM = {{150, 60}, {0, 0}};
const Note SND_BOWSER_JUMP[] PROGMEM = {{110, 80}, {0, 0}};
const Note SND_BOWSER_FIRE[] PROGMEM = {{180, 40}, {140, 60}, {0, 0}};
const Note SND_BRIDGE[] PROGMEM = {{196, 60}, {0, 0}};
const Note SND_DEATH[] PROGMEM = {{494, 150}, {698, 150}, {1, 80}, {698, 150}, {659, 170}, {587, 170},
                                  {523, 200}, {330, 160}, {262, 400}, {0, 0}};
const Note SND_CLEAR[] PROGMEM = {{392, 120}, {523, 120}, {659, 120}, {784, 120}, {1047, 120}, {1319, 120},
                                  {1568, 360}, {1319, 360}, {0, 0}};
const Note SND_START[] PROGMEM = {{659, 100}, {659, 100}, {1, 100}, {659, 100}, {1, 100}, {523, 100},
                                  {659, 200}, {784, 250}, {1, 200}, {392, 300}, {0, 0}};
const Note SND_ENDING[] PROGMEM = {{523, 150}, {659, 150}, {784, 150}, {1047, 300}, {1, 80}, {880, 150},
                                   {1047, 150}, {1319, 600}, {1, 100}, {1175, 150}, {1319, 150}, {1568, 800},
                                   {0, 0}};

}  // namespace mario
