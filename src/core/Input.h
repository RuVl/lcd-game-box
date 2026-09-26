// Джойстик: удержание, нажатия, автоповтор и длинное нажатие. Опрашивается один раз
// за проход loop(). Дребезг, удержание и автоповтор - библиотека EncButton (GyverLibs).
#pragma once

#include <Arduino.h>

namespace Input
{
    // Кнопки - битовые маски, их можно объединять: pressed(UP | CLICK).
    enum : uint8_t { LEFT = 1, RIGHT = 2, UP = 4, DOWN = 8, CLICK = 16 };

    void begin();
    void update();

    bool held(uint8_t mask); // сейчас нажата хотя бы одна из кнопок
    bool pressed(uint8_t mask); // нажата в этом проходе (каждая кнопка из маски - отдельно)
    bool anyPressed(uint8_t mask); // "хотя бы одна из кнопок" перешла из "ни одной" в "нажата"
    bool released(uint8_t mask); // отпущена в этом проходе
    bool repeat(uint8_t mask); // нажата или сработал автоповтор удержания - для меню, тетриса

    int8_t dx(); // -1 влево, 1 вправо, 0 - стик по центру

    uint16_t clickHeldMs(); // сколько уже держат стик нажатым (0 - не нажат)
    uint16_t lastClickMs(); // длительность нажатия, отпущенного в этом проходе

    // Не сообщать о нажатиях, пока все кнопки не отпущены. Вызывается при смене экрана/игры,
    // чтобы нажатие, которым выбрали пункт, не сработало ещё раз на новом экране.
    void suppressUntilRelease();

    // Главное направление стика: одна кнопка из LEFT/RIGHT/UP/DOWN по оси с большим наклоном
    // (или 0). Для игр, где диагональ не нужна и мешает (змейка): наклон влево с лёгким
    // уводом вверх не превратится в два поворота.
    uint8_t direction();

    // Настройки (уровни 1…10), применяются сразу; сохранение - Storage::saveSetting.
    void setSensitivity(uint8_t level); // 1 - почти до упора (95 % хода), 10 - 30 % хода
    void setDebounce(uint8_t level); // × 10 мс

    // Калибровка: центр и крайние значения осей (сырые 0…1023), снимается в JOYSTICK SETUP.
    // Чувствительность считается в процентах реального хода в каждую сторону отдельно - у
    // многих стиков ход в одну сторону короче. Без калибровки ход считается симметричным.
    struct Calibration
    {
        uint16_t minX, midX, maxX, minY, midY, maxY;
    };

    bool calibrated();
    bool validCalibration(const Calibration& c); // центр внутри, в каждую сторону ход не меньше 100
    void setCalibration(const Calibration& c, bool save = true); // применить (и сохранить в EEPROM)

    // Сырые показания осей (0…1023) и центр - для экрана диагностики.
    int rawX();
    int rawY();
    int centerX();
    int centerY();
} // namespace Input
