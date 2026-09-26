// Общие экраны игр: титульный и итог. У всех игр они одинаковые по поведению, поэтому
// живут в ядре один раз - это заметно экономит Flash на UNO.
//
//   TitleScreen title;                        // поле класса игры
//   title.begin("TETRIS", HISCORE_SLOT);      // при входе в состояние "титульный экран"
//   switch (title.update()) {                 // каждый проход
//     case TitleScreen::START: ...; break;    // короткое нажатие стика
//     case TitleScreen::EXIT: requestExit(); break;  // удержание стика
//   }
//
//   ResultScreen result;
//   result.begin("GAME OVER", score, HISCORE_SLOT);   // при входе в состояние "итог"
//   if (result.update()) ...                  // true - пора на титульный экран
#pragma once

#include <Arduino.h>

#include "core/Phase.h"

class TitleScreen
{
public:
    enum Action : uint8_t { NONE, START, EXIT };

    // Строка 0 - название, строка 1 по кругу: PRESS BUTTON, extra (если задана), TOP, HOLD: MENU.
    // Нажатия, зажатые с прошлого экрана, не считаются.
    void begin(const char* title, uint8_t hiScoreSlot, const char* extra = nullptr);
    Action update();

    // Показать строку сразу (например, "IMMORTAL ON"); дальше строки снова идут по кругу.
    void flash(const char* text);
    // Заменить дополнительную строку (например, выбранный режим) и сразу её показать.
    void setExtra(const char* text);

private:
    const char* extra_;
    Phase phase_;
    uint8_t line_;
    char top_[17];

    void showLine();
};

class ResultScreen
{
public:
    // Строка 0 - title, строка 1 - "SCORE 000123" или своя подпись detail.
    // Через 1,5 с рекорд сохраняется (если saveRecord) и показывается "NEW RECORD!".
    void begin(const char* title, uint32_t score, uint8_t hiScoreSlot, const char* detail = nullptr,
               bool saveRecord = true);
    // true - экран закончился (через несколько секунд или по нажатию после проверки рекорда).
    bool update();
    bool newRecord() const { return newRecord_; }

private:
    Phase phase_;
    uint32_t score_;
    uint8_t slot_;
    bool saveRecord_;
    bool newRecord_;
};
