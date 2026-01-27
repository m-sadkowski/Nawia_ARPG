#pragma once

#include "Settings.h"
#include <GlobalScaling.h>
#include <raylib.h>
#include <functional>

namespace Nawia::UI {

/**
 * @class SettingsMenu
 * @brief UI component for game settings (resolution, fullscreen, etc.)
 * 
 * Renders a settings panel with selectable options and handles user input.
 * Uses GlobalScaling for proper sizing at any resolution.
 */
class SettingsMenu {
public:
    /// Callback type for when settings are applied
    using ApplyCallback = std::function<void(const Core::Settings&)>;
    
    /**
     * @brief Construct settings menu with current settings.
     * @param current_settings Reference to current game settings
     */
    explicit SettingsMenu(const Core::Settings& current_settings);
    
    /**
     * @brief Render the settings menu.
     */
    void render(const Font& font) const;
    
    /**
     * @brief Handle input and return action.
     * @return true if user clicked "Back" (close menu)
     */
    bool handleInput();
    
    /**
     * @brief Check if settings were applied.
     * @return true if Apply was clicked this frame
     */
    [[nodiscard]] bool wasApplied() const { return _applied; }
    
    /**
     * @brief Get the modified settings.
     * @return Current settings state (may differ from original if user made changes)
     */
    [[nodiscard]] const Core::Settings& getSettings() const { return _settings; }
    
private:
    Core::Settings _settings;
    int _selected_resolution_index;
    bool _applied = false;
    mutable bool _dragging_slider = false;  ///< Track if slider is being dragged
    
    void drawResolutionSelector(float x, float y, float width, const Font& font) const;
    void drawFullscreenCheckbox(float x, float y, const Font& font) const;
    void drawScaleSlider(float x, float y, float width, const Font& font) const;
    void drawButton(const Rectangle& rect, const char* text, bool is_hovered, const Font& font) const;
   
};

} // namespace Nawia::UI
