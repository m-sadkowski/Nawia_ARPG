#include "SettingsMenu.h"
#include <GlobalScaling.h>

namespace Nawia::UI {

SettingsMenu::SettingsMenu(const Core::Settings& current_settings)
    : _settings(current_settings)
    , _selected_resolution_index(current_settings.getCurrentResolutionIndex())
    , _applied(false)
{
}

void SettingsMenu::render(const Font& font) const {
    const float screen_width = static_cast<float>(GetScreenWidth());
    const float screen_height = static_cast<float>(GetScreenHeight());
    
    // Full screen background
    DrawRectangle(0, 0, static_cast<int>(screen_width), static_cast<int>(screen_height), Fade(BLACK, 0.95f));
    
    // Title at top
    const float title_font_size = Core::GlobalScaling::scaled(50.0f);
    const float spacing = Core::GlobalScaling::scaled(2.0f);
    const char* title = "SETTINGS";
    Vector2 title_size = MeasureTextEx(font, title, title_font_size, spacing);
    DrawTextEx(
        font, 
        title, 
        {(screen_width - title_size.x) / 2.0f, Core::GlobalScaling::scaled(60.0f)}, 
        title_font_size, 
        spacing, 
        DARKGREEN
    );
    
    // Content area - centered
    const float content_width = Core::GlobalScaling::scaled(500.0f);
    const float content_x = (screen_width - content_width) / 2.0f;
    const float content_y = Core::GlobalScaling::scaled(160.0f);
    
    // Resolution label
    const float label_font_size = Core::GlobalScaling::scaled(24.0f);
    DrawTextEx(font, "Resolution:", {content_x, content_y}, label_font_size, spacing, WHITE);
    
    // Resolution selector
    drawResolutionSelector(
        content_x, 
        content_y + Core::GlobalScaling::scaled(40.0f), 
        content_width,
        font
    );
    
    // Fullscreen Checkbox
    const float fullscreen_y = content_y + Core::GlobalScaling::scaled(210.0f);
    drawFullscreenCheckbox(content_x, fullscreen_y, font);

    // UI Scale section
    const float scale_section_y = content_y + Core::GlobalScaling::scaled(250.0f);
    DrawTextEx(font, "UI Scale:", {content_x, scale_section_y}, label_font_size, spacing, WHITE);
    
    drawScaleSlider(
        content_x,
        scale_section_y + Core::GlobalScaling::scaled(40.0f),
        content_width,
        font
    );
    
    // Buttons at bottom
    const float btn_width = Core::GlobalScaling::scaled(150.0f);
    const float btn_height = Core::GlobalScaling::scaled(50.0f);
    const float btn_y = screen_height - Core::GlobalScaling::scaled(120.0f);
    const float btn_spacing = Core::GlobalScaling::scaled(30.0f);
    
    const Vector2 mouse_pos = GetMousePosition();
    
    // Apply button
    const Rectangle apply_btn = {
        (screen_width / 2.0f) - btn_width - btn_spacing / 2.0f,
        btn_y,
        btn_width,
        btn_height
    };
    drawButton(apply_btn, "APPLY", CheckCollisionPointRec(mouse_pos, apply_btn), font);
    
    // Back button
    const Rectangle back_btn = {
        (screen_width / 2.0f) + btn_spacing / 2.0f,
        btn_y,
        btn_width,
        btn_height
    };
    drawButton(back_btn, "BACK", CheckCollisionPointRec(mouse_pos, back_btn), font);
}

void SettingsMenu::drawResolutionSelector(float x, float y, float width, const Font& font) const {
    const float item_height = Core::GlobalScaling::scaled(30.0f);
    const float font_size = Core::GlobalScaling::scaled(16.0f);
    const float spacing = Core::GlobalScaling::scaled(1.0f);
    const Vector2 mouse_pos = GetMousePosition();
    
    for (size_t i = 0; i < Core::Settings::AVAILABLE_RESOLUTIONS.size(); ++i) {
        const auto& res = Core::Settings::AVAILABLE_RESOLUTIONS[i];
        const float item_y = y + static_cast<float>(i) * item_height;
        const Rectangle item_rect = {x, item_y, width, item_height};
        
        const bool is_hovered = CheckCollisionPointRec(mouse_pos, item_rect);
        const bool is_selected = (static_cast<int>(i) == _selected_resolution_index);
        
        // Background
        Color bg_color = is_selected ? Fade(GREEN, 0.3f) : (is_hovered ? Fade(WHITE, 0.1f) : BLANK);
        DrawRectangleRec(item_rect, bg_color);
        
        // Text
        Color text_color = is_selected ? GREEN : (is_hovered ? WHITE : GRAY);
        std::string label = res.toString();
        if (is_selected) label += " *";
        
        DrawTextEx(font, label.c_str(), {x + Core::GlobalScaling::scaled(10.0f), item_y + (item_height - font_size) / 2.0f}, font_size, spacing, text_color);
    }
}

void SettingsMenu::drawFullscreenCheckbox(float x, float y, const Font& font) const {
    const float box_size = Core::GlobalScaling::scaled(24.0f);
    const float font_size = Core::GlobalScaling::scaled(24.0f);
    const float spacing = Core::GlobalScaling::scaled(1.0f);
    const Vector2 mouse_pos = GetMousePosition();

    const Rectangle box_rect = {x, y, box_size, box_size};
    const bool is_hovered = CheckCollisionPointRec(mouse_pos, box_rect);

    // Draw box
    DrawRectangleRec(box_rect, is_hovered ? LIGHTGRAY : DARKGRAY);
    DrawRectangleLinesEx(box_rect, Core::GlobalScaling::scaled(2.0f), WHITE);

    // Draw check if enabled
    if (_settings.fullscreen) {
        DrawRectangleRec({x + box_size * 0.2f, y + box_size * 0.2f, box_size * 0.6f, box_size * 0.6f}, GREEN);
    }

    // Label
    DrawTextEx(font, "Fullscreen", {x + box_size + Core::GlobalScaling::scaled(10.0f), y}, font_size, spacing, WHITE);
}

void SettingsMenu::drawScaleSlider(float x, float y, float width, const Font& font) const {
    const float slider_height = Core::GlobalScaling::scaled(20.0f);
    const float knob_width = Core::GlobalScaling::scaled(20.0f);
    const float font_size = Core::GlobalScaling::scaled(18.0f);
    const float spacing = Core::GlobalScaling::scaled(1.0f);
    
    // Track background
    const Rectangle track_rect = {x, y, width, slider_height};
    DrawRectangleRec(track_rect, DARKGRAY);
    DrawRectangleLinesEx(track_rect, Core::GlobalScaling::scaled(1.0f), GRAY);
    
    // Calculate knob position based on current scale
    const float scale_range = Core::Settings::UI_SCALE_MAX - Core::Settings::UI_SCALE_MIN;
    const float scale_normalized = (_settings.uiScale - Core::Settings::UI_SCALE_MIN) / scale_range;
    const float knob_x = x + scale_normalized * (width - knob_width);
    
    // Knob
    const Rectangle knob_rect = {knob_x, y - Core::GlobalScaling::scaled(2.0f), knob_width, slider_height + Core::GlobalScaling::scaled(4.0f)};
    const Vector2 mouse_pos = GetMousePosition();
    const bool is_hovered = CheckCollisionPointRec(mouse_pos, knob_rect) || _dragging_slider;
    DrawRectangleRec(knob_rect, is_hovered ? WHITE : LIGHTGRAY);
    DrawRectangleLinesEx(knob_rect, Core::GlobalScaling::scaled(1.0f), is_hovered ? GREEN : WHITE);
    
    // Value label
    const auto* value_text = TextFormat("%.1fx", _settings.uiScale);
    Vector2 value_size = MeasureTextEx(font, value_text, font_size, spacing);
    DrawTextEx(
        font,
        value_text,
        {x + width + Core::GlobalScaling::scaled(15.0f), y + (slider_height - value_size.y) / 2.0f},
        font_size,
        spacing,
        WHITE
    );
    
    // Min/Max labels
    const auto* min_text = TextFormat("%.1f", Core::Settings::UI_SCALE_MIN);
    const auto* max_text = TextFormat("%.1f", Core::Settings::UI_SCALE_MAX);
    const float small_font = Core::GlobalScaling::scaled(14.0f);
    DrawTextEx(font, min_text, {x, y + slider_height + Core::GlobalScaling::scaled(5.0f)}, small_font, spacing, GRAY);
    Vector2 max_size = MeasureTextEx(font, max_text, small_font, spacing);
    DrawTextEx(font, max_text, {x + width - max_size.x, y + slider_height + Core::GlobalScaling::scaled(5.0f)}, small_font, spacing, GRAY);
}

void SettingsMenu::drawButton(const Rectangle& rect, const char* text, bool is_hovered, const Font& font) const {
    DrawRectangleRec(rect, is_hovered ? LIGHTGRAY : DARKGRAY);
    DrawRectangleLinesEx(rect, Core::GlobalScaling::scaled(2.0f), WHITE);
    
    const float font_size = Core::GlobalScaling::scaled(18.0f);
    const float spacing = Core::GlobalScaling::scaled(1.0f);
    Vector2 text_size = MeasureTextEx(font, text, font_size, spacing);
    
    DrawTextEx(
        font, 
        text, 
        {rect.x + (rect.width - text_size.x) / 2.0f, rect.y + (rect.height - text_size.y) / 2.0f}, 
        font_size, 
        spacing, 
        is_hovered ? BLACK : WHITE
    );
}

bool SettingsMenu::handleInput() {
    _applied = false;
    
    const float screen_width = static_cast<float>(GetScreenWidth());
    const float screen_height = static_cast<float>(GetScreenHeight());
    const Vector2 mouse_pos = GetMousePosition();
    
    // Content area dimensions (must match render())
    const float content_width = Core::GlobalScaling::scaled(500.0f);
    const float content_x = (screen_width - content_width) / 2.0f;
    const float content_y = Core::GlobalScaling::scaled(160.0f);
    
    // Slider area (must match render())
    const float scale_section_y = content_y + Core::GlobalScaling::scaled(250.0f);
    const float slider_y = scale_section_y + Core::GlobalScaling::scaled(40.0f);
    const float slider_height = Core::GlobalScaling::scaled(20.0f);
    const Rectangle slider_track = {content_x, slider_y, content_width, slider_height};
    
    // Handle slider dragging
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        if (_dragging_slider || CheckCollisionPointRec(mouse_pos, slider_track)) {
            _dragging_slider = true;
            
            // Calculate scale from mouse position
            float relative_x = mouse_pos.x - content_x;
            float normalized = relative_x / content_width;
            normalized = (normalized < 0.0f) ? 0.0f : (normalized > 1.0f) ? 1.0f : normalized;
            
            const float scale_range = Core::Settings::UI_SCALE_MAX - Core::Settings::UI_SCALE_MIN;
            _settings.uiScale = Core::Settings::UI_SCALE_MIN + normalized * scale_range;
            
            // Snap to step
            _settings.uiScale = roundf(_settings.uiScale / Core::Settings::UI_SCALE_STEP) * Core::Settings::UI_SCALE_STEP;
            if (_settings.uiScale < Core::Settings::UI_SCALE_MIN) _settings.uiScale = Core::Settings::UI_SCALE_MIN;
            if (_settings.uiScale > Core::Settings::UI_SCALE_MAX) _settings.uiScale = Core::Settings::UI_SCALE_MAX;
            
            return false;
        }
    } else {
        _dragging_slider = false;
    }
    
    
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        return false;
    }
    
   
    const float res_x = content_x;
    const float res_y = content_y + Core::GlobalScaling::scaled(40.0f);
    const float res_width = content_width;
    const float item_height = Core::GlobalScaling::scaled(30.0f);
    
    for (size_t i = 0; i < Core::Settings::AVAILABLE_RESOLUTIONS.size(); ++i) {
        const float item_y = res_y + static_cast<float>(i) * item_height;
        const Rectangle item_rect = {res_x, item_y, res_width, item_height};
        
        if (CheckCollisionPointRec(mouse_pos, item_rect)) {
            _selected_resolution_index = static_cast<int>(i);
            _settings.setResolutionByIndex(_selected_resolution_index);
            return false;
        }
    }

    
    const float fullscreen_y = content_y + Core::GlobalScaling::scaled(210.0f);
    const float box_size = Core::GlobalScaling::scaled(24.0f);
    const Rectangle box_rect = {content_x, fullscreen_y, box_size, box_size};
    
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse_pos, box_rect)) {
        _settings.fullscreen = !_settings.fullscreen;
        return false;
    }
    
   
    const float btn_width = Core::GlobalScaling::scaled(150.0f);
    const float btn_height = Core::GlobalScaling::scaled(50.0f);
    const float btn_y = screen_height - Core::GlobalScaling::scaled(120.0f);
    const float btn_spacing = Core::GlobalScaling::scaled(30.0f);
    
   
    const Rectangle apply_btn = {
        (screen_width / 2.0f) - btn_width - btn_spacing / 2.0f,
        btn_y,
        btn_width,
        btn_height
    };
    
    if (CheckCollisionPointRec(mouse_pos, apply_btn)) {
        _applied = true;
        return false;
    }
    
    // Back button
    const Rectangle back_btn = {
        (screen_width / 2.0f) + btn_spacing / 2.0f,
        btn_y,
        btn_width,
        btn_height
    };
    
    if (CheckCollisionPointRec(mouse_pos, back_btn)) {
        return true; // Close menu
    }
    
    return false;
}

} // namespace Nawia::UI

