#include "core/Storage.h"

#include <EEPROM.h>

namespace Storage
{
    void begin()
    {
#if defined(ESP8266)
        EEPROM.begin(SLOTS * sizeof(uint32_t) + SETTINGS);
#endif
    }

    uint32_t loadHiScore(uint8_t slot)
    {
        uint32_t v;
        EEPROM.get(slot * sizeof(uint32_t), v);
        return v == 0xFFFFFFFF ? 0 : v; // чистая EEPROM заполнена 0xFF
    }

    void saveHiScore(uint8_t slot, uint32_t score)
    {
        EEPROM.put(slot * sizeof(uint32_t), score);
#if defined(ESP8266)
        EEPROM.commit();
#endif
    }

    namespace
    {
        uint16_t settingAddr(Setting s) { return SLOTS * sizeof(uint32_t) + s; }
    } // namespace

    uint8_t loadSetting(Setting s, uint8_t fallback)
    {
        uint8_t v = EEPROM.read(settingAddr(s));
        return v == 0xFF ? fallback : v;
    }

    void saveSetting(Setting s, uint8_t value)
    {
        if (EEPROM.read(settingAddr(s)) == value) return; // не тратить ресурс EEPROM зря
        EEPROM.write(settingAddr(s), value);
#if defined(ESP8266)
        EEPROM.commit();
#endif
    }

    void loadBlock(Setting s, void* data, uint8_t size)
    {
        uint8_t* p = static_cast<uint8_t*>(data);
        for (uint8_t i = 0; i < size; i++) p[i] = EEPROM.read(settingAddr(s) + i);
    }

    void saveBlock(Setting s, const void* data, uint8_t size)
    {
        const uint8_t* p = static_cast<const uint8_t*>(data);
        for (uint8_t i = 0; i < size; i++)
            if (EEPROM.read(settingAddr(s) + i) != p[i]) EEPROM.write(settingAddr(s) + i, p[i]);
#if defined(ESP8266)
        EEPROM.commit();
#endif
    }
} // namespace Storage
