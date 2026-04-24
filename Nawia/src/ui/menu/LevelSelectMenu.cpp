#include "LevelSelectMenu.h"

#include <GlobalScaling.h>

namespace Nawia::UI {

    LevelSelectMenu::LevelSelectMenu(const std::vector<World::LevelInfo>& levels)
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
        
        Vector2 title_pos = { (screen_w - title_size.x) / 2.0f, Core::GlobalScaling::scaled(40.0f) };
        const float shadow_offset = Core::GlobalScaling::scaled(4.0f);
        DrawTextEx(font, title, { title_pos.x + shadow_offset, title_pos.y + shadow_offset }, title_font_size, spacing, BLACK);
        DrawTextEx(font, title, title_pos, title_font_size, spacing, WHITE);

        // Level cards - arranged horizontally
        const float card_width = Core::GlobalScaling::scaled(250.0f);
        const float card_height = Core::GlobalScaling::scaled(180.0f);
        const float card_spacing = Core::GlobalScaling::scaled(30.0f);

        const float total_width = _levels.size() * card_width + (_levels.size() - 1) * card_spacing;
        float start_x = (screen_w - total_width) / 2.0f;
        const float cards_y = screen_h / 2.0f - card_height / 2.0f;

        const Vector2 mouse_pos = GetMousePosition();

        for (size_t i = 0; i < _levels.size(); ++i) {
            const Rectangle card_rect = { start_x + i * (card_width + card_spacing), cards_y, card_width, card_height };
            const bool hovered = CheckCollisionPointRec(mouse_pos, card_rect);
            drawLevelCard(card_rect, _levels[i], hovered, font);
        }

        // Back button below cards
        const float btn_width = Core::GlobalScaling::scaled(200.0f);
        const float btn_height = Core::GlobalScaling::scaled(50.0f);
        const Rectangle back_rect = { (screen_w - btn_width) / 2.0f, cards_y + card_height + Core::GlobalScaling::scaled(40.0f), btn_width, btn_height };
        const bool back_hovered = CheckCollisionPointRec(mouse_pos, back_rect);
        drawButton(back_rect, "POWROT", back_hovered, font);
    }

    std::string LevelSelectMenu::handleInput() {
        if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            return "";
        }

        const float screen_w = static_cast<float>(GetScreenWidth());
        const float screen_h = static_cast<float>(GetScreenHeight());

        const float card_width = Core::GlobalScaling::scaled(250.0f);
        const float card_height = Core::GlobalScaling::scaled(180.0f);
        const float card_spacing = Core::GlobalScaling::scaled(30.0f);

        const float total_width = _levels.size() * card_width + (_levels.size() - 1) * card_spacing;
        float start_x = (screen_w - total_width) / 2.0f;
        const float cards_y = screen_h / 2.0f - card_height / 2.0f;

        const Vector2 mouse_pos = GetMousePosition();

        for (size_t i = 0; i < _levels.size(); ++i) {
            const Rectangle card_rect = { start_x + i * (card_width + card_spacing), cards_y, card_width, card_height };
            if (CheckCollisionPointRec(mouse_pos, card_rect)) {
                _level_selected = true;
                _selected_level_name = _levels[i].name;
                return _levels[i].name;
            }
        }

        // Back button
        const float btn_width = Core::GlobalScaling::scaled(200.0f);
        const float btn_height = Core::GlobalScaling::scaled(50.0f);
        const Rectangle back_rect = { (screen_w - btn_width) / 2.0f, cards_y + card_height + Core::GlobalScaling::scaled(40.0f), btn_width, btn_height };
        if (CheckCollisionPointRec(mouse_pos, back_rect)) {
            return "BACK";
        }

        return "";
    }

    void LevelSelectMenu::drawLevelCard(const Rectangle& rect, const World::LevelInfo& info, const bool is_hovered, const Font& font) const {
        // Card background
        DrawRectangleRec(rect, is_hovered ? Fade(WHITE, 0.25f) : Fade(WHITE, 0.10f));
        DrawRectangleLinesEx(rect, Core::GlobalScaling::scaled(2.0f), is_hovered ? WHITE : Fade(WHITE, 0.6f));

        // Level name (top of card)
        const float name_font_size = Core::GlobalScaling::scaled(22.0f);
        const float name_spacing = Core::GlobalScaling::scaled(1.0f);
        const Vector2 name_size = MeasureTextEx(font, info.name.c_str(), name_font_size, name_spacing);
        const Vector2 name_pos = {
            rect.x + (rect.width - name_size.x) / 2.0f,
            rect.y + Core::GlobalScaling::scaled(15.0f)
        };
        DrawTextEx(font, info.name.c_str(), name_pos, name_font_size, name_spacing, WHITE);

        // Separator line
        const float sep_y = name_pos.y + name_size.y + Core::GlobalScaling::scaled(10.0f);
        DrawLine(
            static_cast<int>(rect.x + Core::GlobalScaling::scaled(15.0f)),
            static_cast<int>(sep_y),
            static_cast<int>(rect.x + rect.width - Core::GlobalScaling::scaled(15.0f)),
            static_cast<int>(sep_y),
            Fade(WHITE, 0.3f)
        );

        // Location list
        const float loc_font_size = Core::GlobalScaling::scaled(14.0f);
        const float loc_spacing = Core::GlobalScaling::scaled(1.0f);
        float loc_y = sep_y + Core::GlobalScaling::scaled(10.0f);

        // "Lokacje:" label
        const char* label = "Lokacje:";
        const Vector2 label_size = MeasureTextEx(font, label, loc_font_size, loc_spacing);
        DrawTextEx(font, label, { rect.x + Core::GlobalScaling::scaled(15.0f), loc_y }, loc_font_size, loc_spacing, Fade(WHITE, 0.6f));
        loc_y += label_size.y + Core::GlobalScaling::scaled(4.0f);

        for (const auto& loc : info.locations) {
            std::string bullet = "  - " + loc;
            DrawTextEx(font, bullet.c_str(), { rect.x + Core::GlobalScaling::scaled(15.0f), loc_y }, loc_font_size, loc_spacing, Fade(WHITE, 0.8f));
            const Vector2 loc_size = MeasureTextEx(font, bullet.c_str(), loc_font_size, loc_spacing);
            loc_y += loc_size.y + Core::GlobalScaling::scaled(2.0f);
        }
    }

    void LevelSelectMenu::drawButton(const Rectangle& rect, const char* text, const bool is_hovered, const Font& font) const {
        DrawRectangleRec(rect, is_hovered ? Fade(WHITE, 0.4f) : Fade(WHITE, 0.15f));
        DrawRectangleLinesEx(rect, Core::GlobalScaling::scaled(2.0f), WHITE);
        
        const float font_size = Core::GlobalScaling::scaled(20.0f);
        const float spacing = Core::GlobalScaling::scaled(1.0f);
        const Vector2 text_size = MeasureTextEx(font, text, font_size, spacing);
        
        const Vector2 text_pos = { 
            rect.x + (rect.width - text_size.x) / 2.0f, 
            rect.y + (rect.height - text_size.y) / 2.0f 
        };
        
        DrawTextEx(font, text, text_pos, font_size, spacing, WHITE);
    }

} // namespace Nawia::UI

