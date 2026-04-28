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
        const float screen_w = static_cast<float>(GetScreenWidth());
        const float screen_h = static_cast<float>(GetScreenHeight());
        
        // Darkened background for readability
        DrawRectangle(0, 0, (int)screen_w, (int)screen_h, Fade(BLACK, 0.5f));

        const float margin = Core::GlobalScaling::scaled(40.0f);
        const float sidebar_w = Core::GlobalScaling::scaled(280.0f);
        const float content_x = margin + sidebar_w + Core::GlobalScaling::scaled(20.0f);
        const float panel_h = screen_h - (margin * 2.5f);
        const float panel_y = margin;

        drawSidebar(margin, panel_y, sidebar_w, panel_h, ui);
        drawSettingsContent(content_x, panel_y, screen_w - content_x - margin, panel_h, ui);

        // Action Buttons at bottom right, inside the content panel
        const float btn_w = Core::GlobalScaling::scaled(220.0f);
        const float btn_h = Core::GlobalScaling::scaled(60.0f);
        const float btn_margin = Core::GlobalScaling::scaled(25.0f);
        const float btn_y = panel_y + panel_h - btn_h - btn_margin;
        const float btn_spacing = Core::GlobalScaling::scaled(20.0f);

        Rectangle back_rect = { screen_w - margin - btn_w - btn_margin, btn_y, btn_w, btn_h };
        Rectangle apply_rect = { back_rect.x - btn_w - btn_spacing, btn_y, btn_w, btn_h };

        ui.drawMenuButton(apply_rect, "ZATWIERDZ", CheckCollisionPointRec(GetMousePosition(), apply_rect) ? 1.0f : 0.0f);
        ui.drawMenuButton(back_rect, "POWROT", CheckCollisionPointRec(GetMousePosition(), back_rect) ? 1.0f : 0.0f);
    }

    void SettingsMenu::drawSidebar(float x, float y, float width, float height, const UIHandler& ui) const
    {
        // Glass panel
        DrawRectangleRec({x, y, width, height}, Fade(BLACK, 0.4f));
        DrawRectangleLinesEx({x, y, width, height}, 1.0f, Fade(WHITE, 0.2f));

        const char* tabs[] = { "GRAFIKA", "DZWIEK", "STEROWANIE" };
        const Category cats[] = { Category::Graphics, Category::Audio, Category::Controls };
        const float item_h = Core::GlobalScaling::scaled(60.0f);
        const Font& font = ui.getFont();
        const float fs = Core::GlobalScaling::scaled(UIHandler::FONT_SIZE_BUTTON);

        for (int i = 0; i < 3; ++i) {
            Rectangle rect = { x, y + i * item_h, width, item_h };
            bool hovered = CheckCollisionPointRec(GetMousePosition(), rect);
            bool active = (_current_category == cats[i]);

            if (active) {
                DrawRectangleRec(rect, Fade(Color{ 255, 200, 100, 255 }, 0.2f));
                DrawRectangle(static_cast<int>(x), static_cast<int>(rect.y), static_cast<int>(Core::GlobalScaling::scaled(6.0f)), static_cast<int>(item_h), Color{ 255, 200, 100, 255 });
            } else if (hovered) {
                DrawRectangleRec(rect, Fade(WHITE, 0.05f));
            }

            Color text_color = active ? Color{ 255, 200, 100, 255 } : (hovered ? WHITE : GRAY);
            DrawTextEx(font, tabs[i], { x + Core::GlobalScaling::scaled(30.0f), rect.y + (item_h - fs) / 2.0f }, fs, 1.0f, text_color);
        }
    }

    void SettingsMenu::drawSettingsContent(float x, float y, float width, float height, const UIHandler& ui) const
    {
        DrawRectangleRec({x, y, width, height}, Fade(BLACK, 0.25f));
        DrawRectangleLinesEx({x, y, width, height}, 1.0f, Fade(WHITE, 0.1f));

        if (_current_category != Category::Graphics) {
            const char* msg = "Opcja niedostepna w tej wersji";
            Vector2 sz = MeasureTextEx(ui.getFont(), msg, 24.0f, 1.0f);
            DrawTextEx(ui.getFont(), msg, { x + (width - sz.x) / 2.0f, y + (height - sz.y) / 2.0f }, 24.0f, 1.0f, GRAY);
            return;
        }

        float cur_y = y + Core::GlobalScaling::scaled(40.0f);
        float item_w = width - Core::GlobalScaling::scaled(80.0f);
        float item_x = x + Core::GlobalScaling::scaled(40.0f);

        // 1. Resolution
        int res_change = 0;
        drawSelector(item_x, cur_y, item_w, "Rozdzielczosc", _settings.resolution.toString(), ui, &res_change);
        cur_y += Core::GlobalScaling::scaled(80.0f);

        // 2. Fullscreen
        drawToggle(item_x, cur_y, item_w, "Pelny ekran", _settings.fullscreen, ui);
        cur_y += Core::GlobalScaling::scaled(80.0f);

        // 3. UI Scale
        drawSlider(item_x, cur_y, item_w, "Skala interfejsu", _settings.ui_scale, Core::Settings::UI_SCALE_MIN, Core::Settings::UI_SCALE_MAX, ui);
        cur_y += Core::GlobalScaling::scaled(80.0f);

        // 4. Texture Quality
        int tex_change = 0;
        drawSelector(item_x, cur_y, item_w, "Jakosc tekstur", _settings.getTextureQualityString(), ui, &tex_change);
    }

    void SettingsMenu::drawSelector(float x, float y, float width, const char* label, const std::string& value, const UIHandler& ui, int* change_out) const
    {
        const Font& font = ui.getFont();
        const float fs = Core::GlobalScaling::scaled(UIHandler::FONT_SIZE_TEXT);
        const float label_fs = Core::GlobalScaling::scaled(UIHandler::FONT_SIZE_BUTTON);
        
        DrawTextEx(font, label, {x, y + (Core::GlobalScaling::scaled(40.0f) - label_fs) / 2.0f}, label_fs, 1.0f, WHITE);

        const float sel_w = Core::GlobalScaling::scaled(350.0f);
        const float sel_x = x + width - sel_w;
        const Rectangle sel_rect = { sel_x, y, sel_w, Core::GlobalScaling::scaled(40.0f) };
        
        DrawRectangleRec(sel_rect, Fade(WHITE, 0.05f));
        DrawRectangleLinesEx(sel_rect, 1.0f, Fade(WHITE, 0.2f));

        // Arrows
        const float arrow_w = Core::GlobalScaling::scaled(40.0f);
        Rectangle left_arr = { sel_x, y, arrow_w, sel_rect.height };
        Rectangle right_arr = { sel_x + sel_w - arrow_w, y, arrow_w, sel_rect.height };

        bool l_hover = CheckCollisionPointRec(GetMousePosition(), left_arr);
        bool r_hover = CheckCollisionPointRec(GetMousePosition(), right_arr);

        DrawTextEx(font, "<", { left_arr.x + (arrow_w - MeasureTextEx(font, "<", fs, 1.0f).x) / 2.0f, y + (sel_rect.height - fs) / 2.0f }, fs, 1.0f, l_hover ? Color{ 255, 200, 100, 255 } : WHITE);
        DrawTextEx(font, ">", { right_arr.x + (arrow_w - MeasureTextEx(font, ">", fs, 1.0f).x) / 2.0f, y + (sel_rect.height - fs) / 2.0f }, fs, 1.0f, r_hover ? Color{ 255, 200, 100, 255 } : WHITE);

        Vector2 val_sz = MeasureTextEx(font, value.c_str(), fs, 1.0f);
        DrawTextEx(font, value.c_str(), { sel_x + (sel_w - val_sz.x) / 2.0f, y + (sel_rect.height - fs) / 2.0f }, fs, 1.0f, WHITE);
    }

    void SettingsMenu::drawToggle(float x, float y, float width, const char* label, bool enabled, const UIHandler& ui) const
    {
        const Font& font = ui.getFont();
        const float label_fs = Core::GlobalScaling::scaled(UIHandler::FONT_SIZE_BUTTON);
        DrawTextEx(font, label, {x, y + (Core::GlobalScaling::scaled(40.0f) - label_fs) / 2.0f}, label_fs, 1.0f, WHITE);

        const float toggle_w = Core::GlobalScaling::scaled(80.0f);
        const float toggle_h = Core::GlobalScaling::scaled(32.0f);
        const float toggle_x = x + width - toggle_w;
        const float toggle_y = y + (Core::GlobalScaling::scaled(40.0f) - toggle_h) / 2.0f;
        
        Rectangle rect = { toggle_x, toggle_y, toggle_w, toggle_h };
        DrawRectangleRounded(rect, 0.5f, 10, enabled ? Color{ 255, 200, 100, 200 } : DARKGRAY);
        DrawCircleV({ enabled ? (toggle_x + toggle_w - toggle_h / 2.0f - 4) : (toggle_x + toggle_h / 2.0f + 4), toggle_y + toggle_h / 2.0f }, toggle_h / 2.0f - 4, WHITE);
    }

    void SettingsMenu::drawSlider(float x, float y, float width, const char* label, float value, float min, float max, const UIHandler& ui) const
    {
        const Font& font = ui.getFont();
        const float label_fs = Core::GlobalScaling::scaled(UIHandler::FONT_SIZE_BUTTON);
        DrawTextEx(font, label, {x, y + (Core::GlobalScaling::scaled(40.0f) - label_fs) / 2.0f}, label_fs, 1.0f, WHITE);

        const float slider_w = Core::GlobalScaling::scaled(350.0f);
        const float slider_x = x + width - slider_w;
        const float track_h = Core::GlobalScaling::scaled(8.0f);
        const float track_y = y + (Core::GlobalScaling::scaled(40.0f) - track_h) / 2.0f;

        DrawRectangleRounded({slider_x, track_y, slider_w, track_h}, 1.0f, 4, Fade(WHITE, 0.1f));
        
        float pct = (value - min) / (max - min);
        DrawRectangleRounded({slider_x, track_y, slider_w * pct, track_h}, 1.0f, 4, Color{ 255, 200, 100, 255 });

        float knob_r = Core::GlobalScaling::scaled(10.0f);
        DrawCircleV({slider_x + slider_w * pct, track_y + track_h / 2.0f}, knob_r, WHITE);
        
        const char* val_t = TextFormat("%.1fx", value);
        DrawTextEx(font, val_t, { slider_x - Core::GlobalScaling::scaled(60.0f), y + (Core::GlobalScaling::scaled(40.0f) - 18.0f) / 2.0f }, 18.0f, 1.0f, WHITE);
    }

    bool SettingsMenu::handleInput()
    {
        const float screen_w = static_cast<float>(GetScreenWidth());
        const float screen_h = static_cast<float>(GetScreenHeight());
        const float margin = Core::GlobalScaling::scaled(40.0f);
        const float sidebar_w = Core::GlobalScaling::scaled(280.0f);
        const float content_x = margin + sidebar_w + Core::GlobalScaling::scaled(20.0f);
        const float panel_h = screen_h - (margin * 2.5f);
        const float panel_y = margin;
        const Vector2 m = GetMousePosition();

        // Sidebar input
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            const float item_h = Core::GlobalScaling::scaled(60.0f);
            for (int i = 0; i < 3; ++i) {
                if (CheckCollisionPointRec(m, { margin, panel_y + i * item_h, sidebar_w, item_h })) {
                    _current_category = (Category)i;
                    return false;
                }
            }
        }

        if (_current_category == Category::Graphics) {
            float cur_y = panel_y + Core::GlobalScaling::scaled(40.0f);
            float item_w = screen_w - content_x - margin - Core::GlobalScaling::scaled(80.0f);
            float item_x = content_x + Core::GlobalScaling::scaled(40.0f);
            const float sel_w = Core::GlobalScaling::scaled(350.0f);
            const float sel_x = item_x + item_w - sel_w;
            const float arrow_w = Core::GlobalScaling::scaled(40.0f);

            // 1. Resolution
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (CheckCollisionPointRec(m, { sel_x, cur_y, arrow_w, Core::GlobalScaling::scaled(40.0f) })) {
                    _selected_resolution_index = (_selected_resolution_index > 0) ? _selected_resolution_index - 1 : (int)Core::Settings::AVAILABLE_RESOLUTIONS.size() - 1;
                    _settings.setResolutionByIndex(_selected_resolution_index);
                } else if (CheckCollisionPointRec(m, { sel_x + sel_w - arrow_w, cur_y, arrow_w, Core::GlobalScaling::scaled(40.0f) })) {
                    _selected_resolution_index = (_selected_resolution_index < (int)Core::Settings::AVAILABLE_RESOLUTIONS.size() - 1) ? _selected_resolution_index + 1 : 0;
                    _settings.setResolutionByIndex(_selected_resolution_index);
                }
            }
            cur_y += Core::GlobalScaling::scaled(80.0f);

            // 2. Fullscreen
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                const float toggle_w = Core::GlobalScaling::scaled(80.0f);
                if (CheckCollisionPointRec(m, { item_x + item_w - toggle_w, cur_y, toggle_w, Core::GlobalScaling::scaled(40.0f) })) {
                    _settings.fullscreen = !_settings.fullscreen;
                }
            }
            cur_y += Core::GlobalScaling::scaled(80.0f);

            // 3. UI Scale (Slider)
            const float slider_w = Core::GlobalScaling::scaled(350.0f);
            const float slider_x = item_x + item_w - slider_w;
            Rectangle track = { slider_x, cur_y, slider_w, Core::GlobalScaling::scaled(40.0f) };
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                if (_dragging_slider || CheckCollisionPointRec(m, track)) {
                    _dragging_slider = true;
                    float pct = std::clamp((m.x - slider_x) / slider_w, 0.0f, 1.0f);
                    _settings.ui_scale = Core::Settings::UI_SCALE_MIN + pct * (Core::Settings::UI_SCALE_MAX - Core::Settings::UI_SCALE_MIN);
                    _settings.ui_scale = roundf(_settings.ui_scale / Core::Settings::UI_SCALE_STEP) * Core::Settings::UI_SCALE_STEP;
                }
            } else _dragging_slider = false;
            cur_y += Core::GlobalScaling::scaled(80.0f);

            // 4. Texture Quality
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (CheckCollisionPointRec(m, { sel_x, cur_y, arrow_w, Core::GlobalScaling::scaled(40.0f) })) {
                    int val = (int)_settings.texture_quality - 1;
                    if (val < 0) val = 2;
                    _settings.texture_quality = (Core::TextureQuality)val;
                } else if (CheckCollisionPointRec(m, { sel_x + sel_w - arrow_w, cur_y, arrow_w, Core::GlobalScaling::scaled(40.0f) })) {
                    int val = (int)_settings.texture_quality + 1;
                    if (val > 2) val = 0;
                    _settings.texture_quality = (Core::TextureQuality)val;
                }
            }
        }

        // Global buttons
        const float btn_w = Core::GlobalScaling::scaled(220.0f);
        const float btn_h = Core::GlobalScaling::scaled(60.0f);
        const float btn_margin = Core::GlobalScaling::scaled(25.0f);
        const float btn_y = panel_y + panel_h - btn_h - btn_margin;
        const float btn_spacing = Core::GlobalScaling::scaled(20.0f);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (CheckCollisionPointRec(m, { screen_w - margin - btn_w * 2.0f - btn_spacing - btn_margin, btn_y, btn_w, btn_h })) {
                _applied = true;
                return false;
            }
            if (CheckCollisionPointRec(m, { screen_w - margin - btn_w - btn_margin, btn_y, btn_w, btn_h })) return true;
        }

        return false;
    }

} // namespace Nawia::UI
