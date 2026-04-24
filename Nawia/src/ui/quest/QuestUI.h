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

        void render(const Font& font, Game::QuestManager* questManager) const;
        void handleInput();

        void setTab(QuestTab tab) { _currentTab = tab; }
        QuestTab getTab() const { return _currentTab; }

    private:
        QuestTab _currentTab = QuestTab::Active;
        
        // for tracking scroll/selection
        mutable int _selectedQuestIndex = 0;
        
        static constexpr float FONT_SIZE = 20.0f;
        static constexpr float PADDING = 10.0f;
        static constexpr float MENU_WIDTH = 700.0f;
        static constexpr float MENU_HEIGHT = 500.0f;
        
        void drawQuestList(const Font& font, const std::vector<Game::Quest*>& quests, float startX, float startY, float width, float height) const;
        void drawQuestDetails(const Font& font, Game::Quest* quest, float startX, float startY, float width, float height) const;
    };

}
