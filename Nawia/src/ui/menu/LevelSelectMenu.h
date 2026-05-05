#pragma once

#include <LevelManager.h>

#include <raylib.h>
#include <string>
#include <vector>

namespace Nawia::UI {

    class UIHandler;

    /**
     * @class LevelSelectMenu
     * @brief Panel wyboru poziomu z lista dostepnych lokacji.
     */
    class LevelSelectMenu {
    public:
        explicit LevelSelectMenu(const std::vector<World::LevelInfo>& levels);

        /** @brief Rysuje karty poziomow i przycisk powrotu. */
        void render(const UIHandler& ui) const;

        /** @brief Obsluguje klikniecia w karty poziomow. */
        std::string handleInput();

        /** @brief Zwraca, czy gracz wybral poziom. */
        [[nodiscard]] bool wasLevelSelected() const { return _level_selected; }

        /** @brief Zwraca nazwe wybranego poziomu. */
        [[nodiscard]] std::string getSelectedLevelName() const { return _selected_level_name; }

    private:
        std::vector<World::LevelInfo> _levels;
        bool _level_selected = false;
        std::string _selected_level_name;

        /** @brief Rysuje pojedyncza karte poziomu. */
        void drawLevelCard(const Rectangle& rect, const World::LevelInfo& info, bool is_hovered, const Font& font) const;

        /** @brief Rysuje przycisk panelu wyboru poziomu. */
        void drawButton(const Rectangle& rect, const char* text, bool is_hovered, const Font& font) const;
    };

} // namespace Nawia::UI

