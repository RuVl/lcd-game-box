// EEPROM: рекорды игр (слот на 4 байта, адрес = слот × 4), за ними - байтовые настройки.
// На ESP8266 EEPROM эмулируется во Flash: размер задаётся в begin(), запись - через commit().
#pragma once

#include <Arduino.h>

namespace Storage
{
    constexpr uint8_t SLOTS = 16;
    constexpr uint8_t SETTINGS = 16; // байтовые настройки устройства

    // Номера настроек (не менять - хранятся в EEPROM). JOY_CALIBRATION - начало блока
    // калибровки джойстика (loadBlock/saveBlock), он занимает байты до конца области настроек.
    enum Setting : uint8_t { JOY_SENSITIVITY, JOY_DEBOUNCE, JOY_CALIBRATION };

    void begin();
    uint32_t loadHiScore(uint8_t slot); // 0, если ещё не записан
    void saveHiScore(uint8_t slot, uint32_t score);

    uint8_t loadSetting(Setting s, uint8_t fallback); // fallback - если ещё не записана
    void saveSetting(Setting s, uint8_t value);

    // Блок байтов в области настроек, начиная с настройки s (не длиннее, чем до конца области).
    void loadBlock(Setting s, void* data, uint8_t size);
    void saveBlock(Setting s, const void* data, uint8_t size);
} // namespace Storage
