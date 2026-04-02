#pragma once

#include <raylib.h>
#include <string>
#include <vector>

namespace Nawia::UI {

    class LevelSelectMenu {
    public:
        explicit LevelSelectMenu(const std::vector<std::string>& levels);
        
        void render(const Font& font) const;
        std::string handleInput();

        [[nodiscard]] bool wasLevelSelected() const { return _level_selected; }
        [[nodiscard]] std::string getSelectedLevelName() const { return _selected_level_name; }

    private:
        std::vector<std::string> _levels;
        bool _level_selected = false;
        std::string _selected_level_name;

        void drawButton(const Rectangle& rect, const char* text, bool is_hovered, const Font& font) const;
    };

} // namespace Nawia::UI
