#include "SettingsMenu.h"
#include "UIHandler.h"

#include <GlobalScaling.h>

#include <algorithm>
#include <cmath>

namespace Nawia::UI
{

    SettingsMenu::SettingsMenu(const Core::Settings& settings) 
        : _settings(settings)
        , _selected_resolution_index(settings.getCurrentResolutionIndex()) 
    {}

    void SettingsMenu::render(const UIHandler& ui) const
    {
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());
        const float margin = Core::GlobalScaling::scaled(PANEL_MARGIN);
        
        DrawRectangle(0, 0, static_cast<int>(screen_width), static_cast<int>(screen_height), Fade(BLACK, 0.5f));
        
        const float sidebar_width = Core::GlobalScaling::scaled(SIDEBAR_WIDTH);
        const float content_x = margin + sidebar_width + Core::GlobalScaling::scaled(20.0f);
        const float panel_height = screen_height - (margin * 2.5f);
        
        drawSidebar(margin, margin, sidebar_width, panel_height, ui); 
        drawSettingsContent(content_x, margin, screen_width - content_x - margin, panel_height, ui);
        
        // Action Buttons
        const float button_width = Core::GlobalScaling::scaled(BUTTON_WIDTH * 0.65f);
        const float button_height = Core::GlobalScaling::scaled(BUTTON_HEIGHT * 0.85f);
        const float button_margin = Core::GlobalScaling::scaled(25.0f);
        const float button_y = margin + panel_height - button_height - button_margin;
        const float button_spacing = Core::GlobalScaling::scaled(20.0f);
        
        const Rectangle back_rect = { screen_width - margin - button_width - button_margin, button_y, button_width, button_height };
        const Rectangle apply_rect = { back_rect.x - button_width - button_spacing, button_y, button_width, button_height };
        
        ui.drawMenuButton(apply_rect, LABEL_APPLY, CheckCollisionPointRec(GetMousePosition(), apply_rect) ? 1.0f : 0.0f);
        ui.drawMenuButton(back_rect, LABEL_BACK, CheckCollisionPointRec(GetMousePosition(), back_rect) ? 1.0f : 0.0f);
    }

    void SettingsMenu::drawSidebar(float pos_x, float pos_y, float width, float height, const UIHandler& ui) const
    {
        DrawRectangleRec({pos_x, pos_y, width, height}, COLOR_BLACK_GLASS); 
        DrawRectangleLinesEx({pos_x, pos_y, width, height}, 1.0f, Fade(WHITE, 0.2f));
        
        const char* tab_names[] = { "GRAFIKA", "DZWIEK", "STEROWANIE" }; 
        const Category categories[] = { Category::Graphics, Category::Audio, Category::Controls };
        
        const float item_height = Core::GlobalScaling::scaled(60.0f);
        const float font_size = Core::GlobalScaling::scaled(FONT_SIZE_BUTTON);
        
        for (int i = 0; i < 3; ++i)
        {
            const Rectangle item_rect = { pos_x, pos_y + i * item_height, width, item_height }; 
            const bool is_hovered = CheckCollisionPointRec(GetMousePosition(), item_rect);
            const bool is_active = (_current_category == categories[i]);
            
            if (is_active)
            { 
                DrawRectangleRec(item_rect, COLOR_ACCENT_SOFT); 
                DrawRectangle(static_cast<int>(pos_x), static_cast<int>(item_rect.y), static_cast<int>(Core::GlobalScaling::scaled(6.0f)), static_cast<int>(item_height), COLOR_ACCENT); 
            }
            else if (is_hovered)
                DrawRectangleRec(item_rect, COLOR_WHITE_GLASS);
            
            const Color text_color = is_active ? COLOR_ACCENT : (is_hovered ? WHITE : GRAY);
            DrawTextEx(ui.getFont(), tab_names[i], { pos_x + Core::GlobalScaling::scaled(30.0f), item_rect.y + (item_height - font_size) / 2.0f }, font_size, 1.0f, text_color);
        }
    }

    void SettingsMenu::drawSettingsContent(float pos_x, float pos_y, float width, float height, const UIHandler& ui) const
    {
        DrawRectangleRec({pos_x, pos_y, width, height}, Fade(BLACK, 0.25f)); 
        DrawRectangleLinesEx({pos_x, pos_y, width, height}, 1.0f, Fade(WHITE, 0.1f));
        
        if (_current_category != Category::Graphics)
        {
            const char* message = "Opcja niedostepna w tej wersji"; 
            const Vector2 text_size = MeasureTextEx(ui.getFont(), message, 24.0f, 1.0f); 
            DrawTextEx(ui.getFont(), message, { pos_x + (width - text_size.x) / 2.0f, pos_y + (height - text_size.y) / 2.0f }, 24.0f, 1.0f, GRAY); 
            return; 
        }
        
        float current_y = pos_y + Core::GlobalScaling::scaled(40.0f);
        const float item_width = width - Core::GlobalScaling::scaled(80.0f);
        const float item_x = pos_x + Core::GlobalScaling::scaled(40.0f);
        
        int dummy_change = 0; 
        drawSelector(item_x, current_y, item_width, "Rozdzielczosc", _settings.resolution.toString(), ui, &dummy_change); 
        current_y += Core::GlobalScaling::scaled(80.0f);
        
        drawToggle(item_x, current_y, item_width, "Pelny ekran", _settings.fullscreen, ui); 
        current_y += Core::GlobalScaling::scaled(80.0f);
        
        drawSlider(item_x, current_y, item_width, "Skala interfejsu", _settings.ui_scale, Core::Settings::UI_SCALE_MIN, Core::Settings::UI_SCALE_MAX, ui); 
        current_y += Core::GlobalScaling::scaled(80.0f);
        
        int dummy_texture_change = 0; 
        drawSelector(item_x, current_y, item_width, "Jakosc tekstur", _settings.getTextureQualityString(), ui, &dummy_texture_change);
        current_y += Core::GlobalScaling::scaled(80.0f);

        drawToggle(item_x, current_y, item_width, "Licznik FPS", _settings.show_fps, ui);
    }

    void SettingsMenu::drawSelector(float pos_x, float pos_y, float width, const char* label, const std::string& value, const UIHandler& ui, int* change) const
    {
        const Font& font = ui.getFont(); 
        const float text_font_size = Core::GlobalScaling::scaled(FONT_SIZE_TEXT);
        const float label_font_size = Core::GlobalScaling::scaled(FONT_SIZE_BUTTON);
        
        DrawTextEx(font, label, {pos_x, pos_y + (Core::GlobalScaling::scaled(40.0f) - label_font_size) / 2.0f}, label_font_size, 1.0f, WHITE);
        
        const float selector_width = Core::GlobalScaling::scaled(350.0f);
        const float selector_x = pos_x + width - selector_width; 
        const Rectangle selector_rect = { selector_x, pos_y, selector_width, Core::GlobalScaling::scaled(40.0f) };
        
        DrawRectangleRec(selector_rect, COLOR_WHITE_GLASS); 
        DrawRectangleLinesEx(selector_rect, 1.0f, Fade(WHITE, 0.2f));
        
        const float arrow_width = Core::GlobalScaling::scaled(40.0f); 
        const Rectangle left_arrow_rect = { selector_x, pos_y, arrow_width, selector_rect.height };
        const Rectangle right_arrow_rect = { selector_x + selector_width - arrow_width, pos_y, arrow_width, selector_rect.height };
        
        const bool left_hovered = CheckCollisionPointRec(GetMousePosition(), left_arrow_rect);
        const bool right_hovered = CheckCollisionPointRec(GetMousePosition(), right_arrow_rect);
        
        DrawTextEx(font, "<", { left_arrow_rect.x + (arrow_width - MeasureTextEx(font, "<", text_font_size, 1.0f).x) / 2.0f, pos_y + (selector_rect.height - text_font_size) / 2.0f }, text_font_size, 1.0f, left_hovered ? COLOR_ACCENT : WHITE);
        DrawTextEx(font, ">", { right_arrow_rect.x + (arrow_width - MeasureTextEx(font, ">", text_font_size, 1.0f).x) / 2.0f, pos_y + (selector_rect.height - text_font_size) / 2.0f }, text_font_size, 1.0f, right_hovered ? COLOR_ACCENT : WHITE);
        
        const Vector2 value_size = MeasureTextEx(font, value.c_str(), text_font_size, 1.0f); 
        DrawTextEx(font, value.c_str(), { selector_x + (selector_width - value_size.x) / 2.0f, pos_y + (selector_rect.height - text_font_size) / 2.0f }, text_font_size, 1.0f, WHITE);
    }

    void SettingsMenu::drawToggle(float pos_x, float pos_y, float width, const char* label, bool enabled, const UIHandler& ui) const
    {
        const Font& font = ui.getFont(); 
        const float label_font_size = Core::GlobalScaling::scaled(FONT_SIZE_BUTTON); 
        DrawTextEx(font, label, {pos_x, pos_y + (Core::GlobalScaling::scaled(40.0f) - label_font_size) / 2.0f}, label_font_size, 1.0f, WHITE);
        
        const float toggle_width = Core::GlobalScaling::scaled(80.0f);
        const float toggle_height = Core::GlobalScaling::scaled(32.0f);
        const float toggle_x = pos_x + width - toggle_width;
        const float toggle_y = pos_y + (Core::GlobalScaling::scaled(40.0f) - toggle_height) / 2.0f;
        
        const Rectangle toggle_rect = { toggle_x, toggle_y, toggle_width, toggle_height }; 
        DrawRectangleRounded(toggle_rect, 0.5f, 10, enabled ? withAlpha(COLOR_ACCENT, 0.8f) : DARKGRAY); 
        DrawCircleV({ enabled ? (toggle_x + toggle_width - toggle_height / 2.0f - 4) : (toggle_x + toggle_height / 2.0f + 4), toggle_y + toggle_height / 2.0f }, toggle_height / 2.0f - 4, WHITE);
    }

    void SettingsMenu::drawSlider(float pos_x, float pos_y, float width, const char* label, float value, float min_value, float max_value, const UIHandler& ui) const
    {
        const Font& font = ui.getFont(); 
        const float label_font_size = Core::GlobalScaling::scaled(FONT_SIZE_BUTTON); 
        DrawTextEx(font, label, {pos_x, pos_y + (Core::GlobalScaling::scaled(40.0f) - label_font_size) / 2.0f}, label_font_size, 1.0f, WHITE);
        
        const float slider_width = Core::GlobalScaling::scaled(350.0f);
        const float slider_x = pos_x + width - slider_width;
        const float track_height = Core::GlobalScaling::scaled(8.0f);
        const float track_y = pos_y + (Core::GlobalScaling::scaled(40.0f) - track_height) / 2.0f;
        
        DrawRectangleRounded({slider_x, track_y, slider_width, track_height}, 1.0f, 4, COLOR_WHITE_GLASS); 
        
        const float percentage = (value - min_value) / (max_value - min_value); 
        DrawRectangleRounded({slider_x, track_y, slider_width * percentage, track_height}, 1.0f, 4, COLOR_ACCENT);
        
        DrawCircleV({slider_x + slider_width * percentage, track_y + track_height / 2.0f}, Core::GlobalScaling::scaled(10.0f), WHITE); 
        DrawTextEx(font, TextFormat("%.1fx", value), { slider_x - Core::GlobalScaling::scaled(60.0f), pos_y + (Core::GlobalScaling::scaled(40.0f) - 18.0f) / 2.0f }, 18.0f, 1.0f, WHITE);
    }

    bool SettingsMenu::handleInput()
    {
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());
        const float margin = Core::GlobalScaling::scaled(PANEL_MARGIN);
        const float sidebar_width = Core::GlobalScaling::scaled(SIDEBAR_WIDTH);
        const float content_x = margin + sidebar_width + Core::GlobalScaling::scaled(20.0f);
        const float panel_height = screen_height - (margin * 2.5f);
        
        const Vector2 mouse_pos = GetMousePosition();
        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        { 
            const float item_height = Core::GlobalScaling::scaled(60.0f); 
            for (int i = 0; i < 3; ++i)
            {
                if (CheckCollisionPointRec(mouse_pos, { margin, margin + i * item_height, sidebar_width, item_height }))
                { 
                    _current_category = static_cast<Category>(i); 
                    return false; 
                } 
            }
        }
        
        if (_current_category == Category::Graphics)
        {
            float current_y = margin + Core::GlobalScaling::scaled(40.0f);
            const float item_width = screen_width - content_x - margin - Core::GlobalScaling::scaled(80.0f);
            const float item_x = content_x + Core::GlobalScaling::scaled(40.0f);
            const float slider_width = Core::GlobalScaling::scaled(350.0f);
            const float slider_x = item_x + item_width - slider_width;
            const float arrow_width = Core::GlobalScaling::scaled(40.0f);
            
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                if (CheckCollisionPointRec(mouse_pos, { slider_x, current_y, arrow_width, Core::GlobalScaling::scaled(40.0f) }))
                { 
                    _selected_resolution_index = (_selected_resolution_index > 0) ? _selected_resolution_index - 1 : static_cast<int>(Core::Settings::AVAILABLE_RESOLUTIONS.size()) - 1; 
                    _settings.setResolutionByIndex(_selected_resolution_index); 
                }
                else if (CheckCollisionPointRec(mouse_pos, { slider_x + slider_width - arrow_width, current_y, arrow_width, Core::GlobalScaling::scaled(40.0f) }))
                { 
                    _selected_resolution_index = (_selected_resolution_index < static_cast<int>(Core::Settings::AVAILABLE_RESOLUTIONS.size()) - 1) ? _selected_resolution_index + 1 : 0; 
                    _settings.setResolutionByIndex(_selected_resolution_index); 
                }
            }
            current_y += Core::GlobalScaling::scaled(80.0f);
            
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            { 
                const float toggle_width = Core::GlobalScaling::scaled(80.0f); 
                if (CheckCollisionPointRec(mouse_pos, { item_x + item_width - toggle_width, current_y, toggle_width, Core::GlobalScaling::scaled(40.0f) }))
                    _settings.fullscreen = !_settings.fullscreen;
            }
            current_y += Core::GlobalScaling::scaled(80.0f);
            
            const Rectangle track_rect = { slider_x, current_y, slider_width, Core::GlobalScaling::scaled(40.0f) };
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
            { 
                if (_dragging_slider || CheckCollisionPointRec(mouse_pos, track_rect))
                { 
                    _dragging_slider = true; 
                    const float percentage = std::clamp((mouse_pos.x - slider_x) / slider_width, 0.0f, 1.0f); 
                    _settings.ui_scale = Core::Settings::UI_SCALE_MIN + percentage * (Core::Settings::UI_SCALE_MAX - Core::Settings::UI_SCALE_MIN); 
                    _settings.ui_scale = roundf(_settings.ui_scale / Core::Settings::UI_SCALE_STEP) * Core::Settings::UI_SCALE_STEP; 
                } 
            }
            else
                _dragging_slider = false;
            
            current_y += Core::GlobalScaling::scaled(80.0f);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                if (CheckCollisionPointRec(mouse_pos, { slider_x, current_y, arrow_width, Core::GlobalScaling::scaled(40.0f) }))
                { 
                    int value = static_cast<int>(_settings.texture_quality) - 1; 
                    if (value < 0)
                        value = 2; 
                    _settings.texture_quality = static_cast<Core::TextureQuality>(value); 
                }
                else if (CheckCollisionPointRec(mouse_pos, { slider_x + slider_width - arrow_width, current_y, arrow_width, Core::GlobalScaling::scaled(40.0f) }))
                { 
                    int value = static_cast<int>(_settings.texture_quality) + 1; 
                    if (value > 2)
                        value = 0; 
                    _settings.texture_quality = static_cast<Core::TextureQuality>(value); 
                }
            }

            current_y += Core::GlobalScaling::scaled(80.0f);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                const float toggle_width = Core::GlobalScaling::scaled(80.0f);
                if (CheckCollisionPointRec(mouse_pos, { item_x + item_width - toggle_width, current_y, toggle_width, Core::GlobalScaling::scaled(40.0f) }))
                    _settings.show_fps = !_settings.show_fps;
            }
        }
        
        const float button_width = Core::GlobalScaling::scaled(BUTTON_WIDTH * 0.65f);
        const float button_height = Core::GlobalScaling::scaled(BUTTON_HEIGHT * 0.85f);
        const float button_margin = Core::GlobalScaling::scaled(25.0f);
        const float button_y = margin + panel_height - button_height - button_margin;
        const float button_spacing = Core::GlobalScaling::scaled(20.0f);
        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        { 
            if (CheckCollisionPointRec(mouse_pos, { screen_width - margin - button_width * 2.0f - button_spacing - button_margin, button_y, button_width, button_height }))
            { 
                _applied = true; 
                return false; 
            } 
            if (CheckCollisionPointRec(mouse_pos, { screen_width - margin - button_width - button_margin, button_y, button_width, button_height }))
                return true;
        }
        return false;
    }
} // namespace Nawia::UI
