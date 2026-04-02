#include "LevelSelectMenu.h"

#include <GlobalScaling.h>

namespace Nawia::UI {

    LevelSelectMenu::LevelSelectMenu(const std::vector<std::string>& levels)
        : _levels(levels), _level_selected(false) {}

    void LevelSelectMenu::render(const Font& font) const {
        const float screen_w = static_cast<float>(GetScreenWidth());
        const float screen_h = static_cast<float>(GetScreenHeight());

        // Background
        DrawRectangle(0, 0, static_cast<int>(screen_w), static_cast<int>(screen_h), BLACK);

        const float title_font_size = Core::GlobalScaling::scaled(60.0f);
        const float spacing = Core::GlobalScaling::scaled(2.0f);
        const char* title = "WYBIERZ POZIOM";
        const Vector2 title_size = MeasureTextEx(font, title, title_font_size, spacing);
        DrawTextEx(font, title, { (screen_w - title_size.x) / 2.0f, screen_h / 4.0f }, title_font_size, spacing, DARKGREEN);

        const float btn_width = Core::GlobalScaling::scaled(200.0f);
        const float btn_height = Core::GlobalScaling::scaled(50.0f);
        const float btn_spacing = Core::GlobalScaling::scaled(20.0f);
        
        float current_y = screen_h / 2.0f;
        const float center_x = (screen_w - btn_width) / 2.0f;
        const Vector2 mouse_pos = GetMousePosition();

        for (const auto& level_name : _levels) {
            const Rectangle raw_rect = { center_x, current_y, btn_width, btn_height };
            const bool hovered = CheckCollisionPointRec(mouse_pos, raw_rect);
            drawButton(raw_rect, level_name.c_str(), hovered, font);
            current_y += btn_height + btn_spacing;
        }

        // Back button
        const Rectangle back_rect = { center_x, current_y + btn_spacing, btn_width, btn_height };
        const bool back_hovered = CheckCollisionPointRec(mouse_pos, back_rect);
        drawButton(back_rect, "POWROT", back_hovered, font);
    }

    std::string LevelSelectMenu::handleInput() {
        if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            return "";
        }

        const float screen_w = static_cast<float>(GetScreenWidth());
        const float screen_h = static_cast<float>(GetScreenHeight());
        const float btn_width = Core::GlobalScaling::scaled(200.0f);
        const float btn_height = Core::GlobalScaling::scaled(50.0f);
        const float btn_spacing = Core::GlobalScaling::scaled(20.0f);
        
        float current_y = screen_h / 2.0f;
        const float center_x = (screen_w - btn_width) / 2.0f;
        const Vector2 mouse_pos = GetMousePosition();

        for (const auto& level_name : _levels) {
            const Rectangle raw_rect = { center_x, current_y, btn_width, btn_height };
            if (CheckCollisionPointRec(mouse_pos, raw_rect)) {
                _level_selected = true;
                _selected_level_name = level_name;
                return level_name;
            }
            current_y += btn_height + btn_spacing;
        }

        const Rectangle back_rect = { center_x, current_y + btn_spacing, btn_width, btn_height };
        if (CheckCollisionPointRec(mouse_pos, back_rect)) {
            return "BACK";
        }

        return "";
    }

    void LevelSelectMenu::drawButton(const Rectangle& rect, const char* text, const bool is_hovered, const Font& font) const {
        DrawRectangleRec(rect, is_hovered ? LIGHTGRAY : RAYWHITE);
        DrawRectangleLinesEx(rect, Core::GlobalScaling::scaled(2.0f), BLACK);
        
        const float font_size = Core::GlobalScaling::scaled(20.0f);
        const float spacing = Core::GlobalScaling::scaled(1.0f);
        const Vector2 text_size = MeasureTextEx(font, text, font_size, spacing);
        
        const Vector2 text_pos = { 
            rect.x + (rect.width - text_size.x) / 2.0f, 
            rect.y + (rect.height - text_size.y) / 2.0f 
        };
        
        DrawTextEx(font, text, text_pos, font_size, spacing, BLACK);
    }

} // namespace Nawia::UI
