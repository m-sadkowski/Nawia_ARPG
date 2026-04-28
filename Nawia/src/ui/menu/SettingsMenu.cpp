#include "SettingsMenu.h"
#include "UIHandler.h"
#include <GlobalScaling.h>
#include <algorithm>
#include <cmath>

namespace Nawia::UI {

	SettingsMenu::SettingsMenu(const Core::Settings& current_settings)
	    : _settings(current_settings)
	    , _selected_resolution_index(current_settings.getCurrentResolutionIndex())
	    , _applied(false)
	{
	}

	void SettingsMenu::render(const UIHandler& ui) const 
	{
	    const float screen_width = static_cast<float>(GetScreenWidth());
	    const float screen_height = static_cast<float>(GetScreenHeight());
	    const Font& font = ui.getFont();
        const float spacing = Core::GlobalScaling::scaled(2.0f);
	    
	    // Semi-transparent overlay to help text readability
	    DrawRectangle(0, 0, static_cast<int>(screen_width), static_cast<int>(screen_height), Fade(BLACK, 0.4f));
	    
	    // Title at top
	    const float title_font_size = Core::GlobalScaling::scaled(UIHandler::FONT_SIZE_TITLE);
	    const char* title = "USTAWIENIA";
	    Vector2 title_size = MeasureTextEx(font, title, title_font_size, spacing);
	    DrawTextEx(font, title, {(screen_width - title_size.x) / 2.0f, Core::GlobalScaling::scaled(60.0f)}, title_font_size, spacing, { 255, 200, 100, 255 });
	    
	    // Content area - centered
	    const float content_width = Core::GlobalScaling::scaled(500.0f);
	    const float content_x = (screen_width - content_width) / 2.0f;
	    const float content_y = Core::GlobalScaling::scaled(180.0f);
	    
	    // Resolution label
	    const float label_font_size = Core::GlobalScaling::scaled(UIHandler::FONT_SIZE_SUBTITLE);
	    DrawTextEx(font, "Rozdzielczosc:", {content_x, content_y}, label_font_size, spacing, WHITE);
	    
	    // Resolution selector
	    drawResolutionSelector(content_x, content_y + Core::GlobalScaling::scaled(50.0f), content_width, font);
	    
	    // Fullscreen Checkbox
	    const float fullscreen_y = content_y + Core::GlobalScaling::scaled(230.0f);
	    drawFullscreenCheckbox(content_x, fullscreen_y, font);

	    // UI Scale section
	    const float scale_section_y = content_y + Core::GlobalScaling::scaled(280.0f);
	    DrawTextEx(font, "Skala interfejsu:", {content_x, scale_section_y}, label_font_size, spacing, WHITE);
	    
	    drawScaleSlider(content_x, scale_section_y + Core::GlobalScaling::scaled(50.0f), content_width, font);
	    
	    // Buttons at bottom
	    const float btn_width = Core::GlobalScaling::scaled(220.0f);
	    const float btn_height = Core::GlobalScaling::scaled(60.0f);
	    const float btn_y = screen_height - Core::GlobalScaling::scaled(140.0f);
	    const float btn_spacing = Core::GlobalScaling::scaled(40.0f);
	    
	    const Vector2 mouse_pos = GetMousePosition();
	    
	    // Apply button
	    const Rectangle apply_btn = { (screen_width / 2.0f) - btn_width - btn_spacing / 2.0f, btn_y, btn_width, btn_height };
	    ui.drawMenuButton(apply_btn, "ZATWIERDZ", CheckCollisionPointRec(mouse_pos, apply_btn) ? 1.0f : 0.0f);
	    
	    // Back button
	    const Rectangle back_btn = { (screen_width / 2.0f) + btn_spacing / 2.0f, btn_y, btn_width, btn_height };
	    ui.drawMenuButton(back_btn, "POWROT", CheckCollisionPointRec(mouse_pos, back_btn) ? 1.0f : 0.0f);
	}

	void SettingsMenu::drawResolutionSelector(const float x, const float y, const float width, const Font& font) const
	{
	    const float item_height = Core::GlobalScaling::scaled(35.0f);
	    const float font_size = Core::GlobalScaling::scaled(UIHandler::FONT_SIZE_TEXT);
	    const float spacing = Core::GlobalScaling::scaled(1.0f);
	    const Vector2 mouse_pos = GetMousePosition();
	    
	    for (size_t i = 0; i < Core::Settings::AVAILABLE_RESOLUTIONS.size(); ++i) 
		{
	        const auto& res = Core::Settings::AVAILABLE_RESOLUTIONS[i];
	        const float item_y = y + static_cast<float>(i) * item_height;
	        const Rectangle item_rect = {x, item_y, width, item_height};
	        
	        const bool is_hovered = CheckCollisionPointRec(mouse_pos, item_rect);
	        const bool is_selected = (static_cast<int>(i) == _selected_resolution_index);
	        
	        // Background
	        Color bg_color = is_selected ? Fade({ 255, 200, 100, 255 }, 0.3f) : (is_hovered ? Fade(WHITE, 0.1f) : BLANK);
	        DrawRectangleRec(item_rect, bg_color);
	        
	        // Text
	        Color text_color = is_selected ? Color{ 255, 220, 150, 255 } : (is_hovered ? WHITE : GRAY);
	        std::string label = res.toString();
	        if (is_selected) label = "> " + label;
	        
	        DrawTextEx(font, label.c_str(), {x + Core::GlobalScaling::scaled(15.0f), item_y + (item_height - font_size) / 2.0f}, font_size, spacing, text_color);
	    }
	}

	void SettingsMenu::drawFullscreenCheckbox(const float x, const float y, const Font& font) const
	{
	    const float box_size = Core::GlobalScaling::scaled(28.0f);
	    const float font_size = Core::GlobalScaling::scaled(UIHandler::FONT_SIZE_TEXT);
	    const float spacing = Core::GlobalScaling::scaled(1.0f);
	    const Vector2 mouse_pos = GetMousePosition();

	    const Rectangle box_rect = {x, y, box_size, box_size};
	    const bool is_hovered = CheckCollisionPointRec(mouse_pos, box_rect);

	    DrawRectangleRec(box_rect, is_hovered ? Fade(WHITE, 0.3f) : Fade(WHITE, 0.1f));
	    DrawRectangleLinesEx(box_rect, Core::GlobalScaling::scaled(2.0f), WHITE);

	    if (_settings.fullscreen) {
	        DrawRectangleRec({x + box_size * 0.2f, y + box_size * 0.2f, box_size * 0.6f, box_size * 0.6f}, { 255, 200, 100, 255 });
	    }

	    DrawTextEx(font, "Pelny ekran", {x + box_size + Core::GlobalScaling::scaled(15.0f), y + (box_size - font_size) / 2.0f}, font_size, spacing, WHITE);
	}

	void SettingsMenu::drawScaleSlider(const float x, const float y, const float width, const Font& font) const
	{
	    const float slider_height = Core::GlobalScaling::scaled(12.0f);
	    const float knob_width = Core::GlobalScaling::scaled(24.0f);
	    const float font_size = Core::GlobalScaling::scaled(UIHandler::FONT_SIZE_TEXT);
	    const float spacing = Core::GlobalScaling::scaled(1.0f);
	    
	    const Rectangle track_rect = {x, y + (knob_width - slider_height) / 2.0f, width, slider_height};
	    DrawRectangleRec(track_rect, Fade(WHITE, 0.15f));
	    DrawRectangleLinesEx(track_rect, Core::GlobalScaling::scaled(1.0f), Fade(WHITE, 0.4f));
	    
	    constexpr float scale_range = Core::Settings::UI_SCALE_MAX - Core::Settings::UI_SCALE_MIN;
	    const float scale_normalized = (_settings.ui_scale - Core::Settings::UI_SCALE_MIN) / scale_range;
	    const float knob_x = x + scale_normalized * (width - knob_width);
	    
	    const Rectangle knob_rect = {knob_x, y, knob_width, knob_width};
	    const Vector2 mouse_pos = GetMousePosition();
	    const bool is_hovered = CheckCollisionPointRec(mouse_pos, knob_rect) || _dragging_slider;
	    DrawRectangleRec(knob_rect, is_hovered ? Color{ 255, 220, 100, 255 } : WHITE);
	    
	    const auto* value_text = TextFormat("%.1fx", _settings.ui_scale);
	    DrawTextEx(font, value_text, {x + width + Core::GlobalScaling::scaled(20.0f), y + (knob_width - font_size) / 2.0f}, font_size, spacing, WHITE);
	}

	bool SettingsMenu::handleInput() 
	{
	    _applied = false;
	    const float screen_width = static_cast<float>(GetScreenWidth());
	    const float screen_height = static_cast<float>(GetScreenHeight());
	    const Vector2 mouse_pos = GetMousePosition();
	    
	    const float content_width = Core::GlobalScaling::scaled(500.0f);
	    const float content_x = (screen_width - content_width) / 2.0f;
	    const float content_y = Core::GlobalScaling::scaled(180.0f);
	    
	    const float scale_section_y = content_y + Core::GlobalScaling::scaled(280.0f);
	    const float knob_size = Core::GlobalScaling::scaled(24.0f);
	    const Rectangle slider_track = {content_x, scale_section_y + Core::GlobalScaling::scaled(50.0f), content_width, knob_size};
	    
	    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) 
		{
	        if (_dragging_slider || CheckCollisionPointRec(mouse_pos, slider_track))
			{
	            _dragging_slider = true;
				const float relative_x = mouse_pos.x - content_x;
	            float normalized = relative_x / content_width;
	            normalized = std::clamp(normalized, 0.0f, 1.0f);
	            
	            constexpr float scale_range = Core::Settings::UI_SCALE_MAX - Core::Settings::UI_SCALE_MIN;
	            _settings.ui_scale = Core::Settings::UI_SCALE_MIN + normalized * scale_range;
	            _settings.ui_scale = roundf(_settings.ui_scale / Core::Settings::UI_SCALE_STEP) * Core::Settings::UI_SCALE_STEP;
	            _settings.ui_scale = std::clamp(_settings.ui_scale, Core::Settings::UI_SCALE_MIN, Core::Settings::UI_SCALE_MAX);
	            return false;
	        }
	    } else {
	        _dragging_slider = false;
	    }
	    
	    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return false;
	    
	    const float item_height = Core::GlobalScaling::scaled(35.0f);
	    for (size_t i = 0; i < Core::Settings::AVAILABLE_RESOLUTIONS.size(); ++i) {
	        const Rectangle item_rect = {content_x, content_y + Core::GlobalScaling::scaled(50.0f) + static_cast<float>(i) * item_height, content_width, item_height};
	        if (CheckCollisionPointRec(mouse_pos, item_rect)) {
	            _selected_resolution_index = static_cast<int>(i);
	            _settings.setResolutionByIndex(_selected_resolution_index);
	            return false;
	        }
	    }

	    const float fullscreen_y = content_y + Core::GlobalScaling::scaled(230.0f);
	    const float box_size = Core::GlobalScaling::scaled(28.0f);
	    if (CheckCollisionPointRec(mouse_pos, {content_x, fullscreen_y, box_size, box_size})) {
	        _settings.fullscreen = !_settings.fullscreen;
	        return false;
	    }
	    
	    const float btn_width = Core::GlobalScaling::scaled(220.0f);
	    const float btn_height = Core::GlobalScaling::scaled(60.0f);
	    const float btn_y = screen_height - Core::GlobalScaling::scaled(140.0f);
	    const float btn_spacing = Core::GlobalScaling::scaled(40.0f);
	    
	    if (CheckCollisionPointRec(mouse_pos, { (screen_width / 2.0f) - btn_width - btn_spacing / 2.0f, btn_y, btn_width, btn_height })) {
	        _applied = true;
	        return false;
	    }
	    
	    if (CheckCollisionPointRec(mouse_pos, { (screen_width / 2.0f) + btn_spacing / 2.0f, btn_y, btn_width, btn_height })) return true;
	    
	    return false;
	}

} // namespace Nawia::UI
