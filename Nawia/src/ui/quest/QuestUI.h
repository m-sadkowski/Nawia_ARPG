#pragma once

#include <raylib.h>
#include <string>
#include <vector>

namespace Nawia::Game {
    class QuestManager;
    class Quest;
}

namespace Nawia::UI {

    enum class QuestTab {
        Active,
        Completed
    };

    class QuestUI {
    public:
        QuestUI();
        ~QuestUI() = default;

        static constexpr float MENU_WIDTH = 700.0f;
        static constexpr float MENU_HEIGHT = 500.0f;

        /** @brief Rysuje panel questow z lista i szczegolami wybranego zadania. */
        void render(const Font& font, Game::QuestManager* quest_manager) const;

        /** @brief Obsluguje zakladki i wybor questu z listy. */
        void handleInput();

        /** @brief Ustawia aktywna zakladke questow. */
        void set_tab(QuestTab tab) { _current_tab = tab; }

        /** @brief Zwraca aktywna zakladke questow. */
        QuestTab get_tab() const { return _current_tab; }

    private:
        QuestTab _current_tab = QuestTab::Active;
        
        // Indeks aktualnie wybranego questa.
        mutable int _selected_quest_index = 0;
        
        static constexpr float FONT_SIZE = 20.0f;
        static constexpr float PADDING = 10.0f;
        
        /** @brief Rysuje lewa kolumne z lista questow. */
        void drawQuestList(const Font& font, const std::vector<Game::Quest*>& quests, float start_x, float start_y, float width, float height) const;

        /** @brief Rysuje prawa kolumne ze szczegolami wybranego questa. */
        void drawQuestDetails(const Font& font, Game::Quest* quest, float start_x, float start_y, float width, float height) const;
    };

} // namespace Nawia::UI
