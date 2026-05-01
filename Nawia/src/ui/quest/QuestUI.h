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

        void render(const Font& font, Game::QuestManager* quest_manager) const;
        void handleInput();

        void set_tab(QuestTab tab) { _current_tab = tab; }
        QuestTab get_tab() const { return _current_tab; }

    private:
        QuestTab _current_tab = QuestTab::Active;
        
        // for tracking scroll/selection
        mutable int _selected_quest_index = 0;
        
        static constexpr float FONT_SIZE = 20.0f;
        static constexpr float PADDING = 10.0f;
        
        void drawQuestList(const Font& font, const std::vector<Game::Quest*>& quests, float start_x, float start_y, float width, float height) const;
        void drawQuestDetails(const Font& font, Game::Quest* quest, float start_x, float start_y, float width, float height) const;
    };

}
