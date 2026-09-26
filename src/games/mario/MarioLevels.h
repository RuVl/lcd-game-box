// Mario: уровни в PROGMEM. Подключается только из MarioGame.cpp.
//
// Верхний ряд: ' ' пусто, 'o' монета, '?' блок с монетой, 'M' блок с грибом,
//              '#' кирпич (под ним не прыгнуть).
//              Во время игры ещё: 'c'/'m' — монета/гриб вылетает из блока, 'u' — пустой блок.
// Нижний ряд:  '_' земля, ' ' яма (в замке — лава), 'P' труба, 'T' труба с пираньей,
//              'G' гумба, 'K' купа, 'L' движущаяся платформа (стоит в яме, «LL» — ширина 2),
//              'S' пружина, 'f' лава с огненным шаром, '|' флагшток,
//              '=' мост, 'B' Боузер (стоит на мосту), 'A' топор (цель в замке).
// Уровни спроектированы и проверены на проходимость скриптом, повторяющим gameTick().
// Каждому уровню нужно не больше 7 пользовательских символов (8-й — Марио).
//
// После правки уровней запустите проверку: python3 tools/check_levels.py
#pragma once

#include "games/mario/MarioTypes.h"

namespace mario {

const char L1_TOP[] PROGMEM =
    "      ooo      M        oo     oo    ?o o           ooo        ?    o o         ooo      ???                       ";
const char L1_BOT[] PROGMEM =
    "____________P_____G_____  _____PP___G___P____G__P___   ____G__G______ _ G____PP__PPP_______________|_______________";
const char L2_TOP[] PROGMEM =
    "   ##########  o  #####      ooo    ######## o o ###?           ooo    #######  o o o          ??                      ";
const char L2_BOT[] PROGMEM =
    "_______________  _________P_G____P___________   ______P___G___PPP _P___________ _ _ __P__G__P_______  ____|____________";
const char L3_TOP[] PROGMEM =
    "   o o   oo      o       ooo            oo o       o            o        oo                        ";
const char L3_BOT[] PROGMEM =
    "________   ___P   P__K__  _  _P___G___P LL    ____ _ ____K___P  _  __P_  LL _G__P  ___|____________";
const char L4_TOP[] PROGMEM =
    "   ###########  o  #####      ####  o o     #####                               ";
const char L4_BOT[] PROGMEM =
    "________________f_________  _______ f _ __________f___=====B======A_____________";
const char L5_TOP[] PROGMEM =
    "        o       oo          ooo                         o o                              ";
const char L5_BOT[] PROGMEM =
    "________T___G___  ____P____K_____T__   __T___G__G___PT__ _ ___K___K_T_  ____|____________";
const char L6_TOP[] PROGMEM =
    "    ?M     o o o        ?            oooooo      ?   ?               o                        ";
const char L6_BOT[] PROGMEM =
    "_________S        __G______  __G___S        _____________ ___G___S       ________|____________";
const char L7_TOP[] PROGMEM =
    "   #########   o   #####          ####### o  o               ooo      ####                           ";
const char L7_BOT[] PROGMEM =
    "_____________ LL  _______ _K__ ___________ LL    ____G_____  LL   __________ _K__K_ ____|____________";
const char L8_TOP[] PROGMEM =
    "         o o                oo        ooo                              ";
const char L8_BOT[] PROGMEM =
    "______S        __T_  _G__T  LL   __S        _G_T_  _______|____________";
const char L9_TOP[] PROGMEM =
    "   ##########  o  ######     o  ####        #####                                     ";
const char L9_BOT[] PROGMEM =
    "_______________f_________ __f_f_______ff__________ f____=======B========A_____________";

const LevelDef LEVELS[] PROGMEM = {
    {L1_TOP, L1_BOT, 1, 1, 3, false, false},
    {L2_TOP, L2_BOT, 1, 2, 3, false, false},
    {L3_TOP, L3_BOT, 1, 3, 2, false, false},
    {L4_TOP, L4_BOT, 1, 4, 2, true, false},
    {L5_TOP, L5_BOT, 2, 1, 2, false, false},
    {L6_TOP, L6_BOT, 2, 2, 2, false, false},
    {L7_TOP, L7_BOT, 2, 3, 2, false, false},
    {L8_TOP, L8_BOT, 2, 4, 2, false, false},
    {L9_TOP, L9_BOT, 2, 5, 2, true, true},
};
constexpr uint8_t LEVEL_COUNT = sizeof(LEVELS) / sizeof(LEVELS[0]);

}  // namespace mario
