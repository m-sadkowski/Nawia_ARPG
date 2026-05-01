#pragma once

#include <LevelManager.h>

#include <raylib.h>
#include <string>
#include <vector>

namespace Nawia::UI {

    class UIHandler;

    class LevelSelectMenu {
    public:
        explicit LevelSelectMenu(const std::vector<World::LevelInfo>& levels);
        
        void render(const UIHandler& ui) const;
        std::string handleInput();

        [[nodiscard]] bool wasLevelSelected() const { return _level_selected; }
        [[nodiscard]] std::string getSelectedLevelName() const { return _selected_level_name; }

    private:
        std::vector<World::LevelInfo> _levels;
        bool _level_selected = false;
        std::string _selected_level_name;

        void drawLevelCard(const Rectangle& rect, const World::LevelInfo& info, bool is_hovered, const Font& font) const;
        void drawButton(const Rectangle& rect, const char* text, bool is_hovered, const Font& font) const;
    };

} // namespace Nawia::UI

