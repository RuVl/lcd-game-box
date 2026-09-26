#include "core/Ticker.h"

bool Ticker::due(uint16_t periodMs)
{
    if ((int16_t)((uint16_t)millis() - next_) < 0) return false;
    next_ += periodMs;
    return true;
}
