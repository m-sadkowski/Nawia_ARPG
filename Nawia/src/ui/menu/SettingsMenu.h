#pragma once

#include <Settings.h>
#include <GlobalScaling.h>

#include <raylib.h>
#include <functional>
#include <string>
#include <vector>

namespace Nawia::UI {

    class UIHandler;

/**
 * @class SettingsMenu
 * @brief UI component for game settings (resolution, fullscreen, etc.)
 * 
 * Renders a premium settings panel with categories and modern sliders/selectors.
 */
class SettingsMenu {
public:
    enum class Category { Graphics, Audio, Controls };

    explicit SettingsMenu(const Core::Settings& current_settings);
    
    void render(const UIHandler& ui) const;
    bool handleInput();
    
    [[nodiscard]] bool wasApplied() const { return _applied; }
    [[nodiscard]] const Core::Settings& getSettings() const { return _settings; }
    
private:
    Core::Settings _settings;
    int _selected_resolution_index;
    Category _current_category = Category::Graphics;
    bool _applied = false;
    mutable bool _dragging_slider = false;

    // Helpers for modern UI
    void drawSidebar(float x, float y, float width, float height, const UIHandler& ui) const;
    void drawSettingsContent(float x, float y, float width, float height, const UIHandler& ui) const;
    
    // Modern controls
    void drawSelector(float x, float y, float width, const char* label, const std::string& value, const UIHandler& ui, int* change_out) const;
    void drawToggle(float x, float y, float width, const char* label, bool enabled, const UIHandler& ui) const;
    void drawSlider(float x, float y, float width, const char* label, float value, float min, float max, const UIHandler& ui) const;
};

} // namespace Nawia::UI
