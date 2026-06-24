#pragma once

#include <GlobalScaling.h>
#include <Settings.h>

#include <raylib.h>
#include <functional>
#include <string>
#include <vector>

namespace Nawia::UI {

    class UIHandler;

    /**
     * @class SettingsMenu
     * @brief Panel ustawien gry z rozdzielczoscia, fullscreenem i skala UI.
     */
    class SettingsMenu {
    public:
        /** @brief Kategorie ustawien widoczne w panelu bocznym. */
        enum class Category { Graphics, Audio, Controls };

        /** @brief Sposob prezentacji wartosci slidera. */
        enum class SliderDisplay { Multiplier, Percent };

        explicit SettingsMenu(const Core::Settings& current_settings);

        /** @brief Rysuje panel ustawien. */
        void render(const UIHandler& ui) const;

        /** @brief Obsluguje klikniecia i zmiany kontrolek. */
        bool handleInput();

        /** @brief Zwraca, czy ustawienia zostaly zatwierdzone. */
        [[nodiscard]] bool wasApplied() const { return _applied; }

        /** @brief Zwraca aktualnie wybrane ustawienia. */
        [[nodiscard]] const Core::Settings& getSettings() const { return _settings; }

    private:
        Core::Settings _settings;
        int _selected_resolution_index;
        Category _current_category = Category::Graphics;
        bool _applied = false;
        int _dragging_slider_index = -1;

        void drawSidebar(float x, float y, float width, float height, const UIHandler& ui) const;
        void drawSettingsContent(float x, float y, float width, float height, const UIHandler& ui) const;
        void drawSelector(float x, float y, float width, const char* label, const std::string& value, const UIHandler& ui, int* change_out) const;
        void drawToggle(float x, float y, float width, const char* label, bool enabled, const UIHandler& ui) const;
        void drawSlider(float x, float y, float width, const char* label, float value, float min, float max, const UIHandler& ui, SliderDisplay display) const;
    };

} // namespace Nawia::UI
