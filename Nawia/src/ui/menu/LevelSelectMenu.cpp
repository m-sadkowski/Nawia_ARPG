#include "LevelSelectMenu.h"
#include "UIHandler.h"
#include <GlobalScaling.h>

namespace Nawia::UI {

    LevelSelectMenu::LevelSelectMenu(const std::vector<World::LevelInfo>& levels)
        : _levels(levels), _level_selected(false) {}

    void LevelSelectMenu::render(const UIHandler& ui) const {
        const float screen_w = static_cast<float>(GetScreenWidth());
        const float screen_h = static_cast<float>(GetScreenHeight());
        const Font& font = ui.getFont();
        const float spacing = Core::GlobalScaling::scaled(2.0f);

        // Semi-transparent overlay
        DrawRectangle(0, 0, static_cast<int>(screen_w), static_cast<int>(screen_h), Fade(BLACK, 0.4f));

        const float title_font_size = Core::GlobalScaling::scaled(UIHandler::FONT_SIZE_TITLE);
        const char* title = "WYBIERZ POZIOM";
        const Vector2 title_size = MeasureTextEx(font, title, title_font_size, spacing);
        DrawTextEx(font, title, {(screen_w - title_size.x) / 2.0f, Core::GlobalScaling::scaled(60.0f)}, title_font_size, spacing, { 255, 200, 100, 255 });

        // Level cards - arranged horizontally
        const float card_width = Core::GlobalScaling::scaled(280.0f);
        const float card_height = Core::GlobalScaling::scaled(200.0f);
        const float card_spacing = Core::GlobalScaling::scaled(40.0f);

        const float total_width = _levels.size() * card_width + (_levels.size() - 1) * card_spacing;
        float start_x = (screen_w - total_width) / 2.0f;
        const float cards_y = screen_h / 2.0f - card_height / 2.0f;

        const Vector2 mouse_pos = GetMousePosition();

        for (size_t i = 0; i < _levels.size(); ++i) {
            const Rectangle card_rect = { start_x + i * (card_width + card_spacing), cards_y, card_width, card_height };
            const bool hovered = CheckCollisionPointRec(mouse_pos, card_rect);
            drawLevelCard(card_rect, _levels[i], hovered, font);
        }

        // Back button at bottom center
        const float btn_width = Core::GlobalScaling::scaled(220.0f);
        const float btn_height = Core::GlobalScaling::scaled(60.0f);
        const Rectangle back_rect = { (screen_w - btn_width) / 2.0f, screen_h - Core::GlobalScaling::scaled(140.0f), btn_width, btn_height };
        ui.drawMenuButton(back_rect, "POWROT", CheckCollisionPointRec(mouse_pos, back_rect) ? 1.0f : 0.0f);
    }

    std::string LevelSelectMenu::handleInput() {
        if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return "";

        const float screen_w = static_cast<float>(GetScreenWidth());
        const float screen_h = static_cast<float>(GetScreenHeight());

        const float card_width = Core::GlobalScaling::scaled(280.0f);
        const float card_height = Core::GlobalScaling::scaled(200.0f);
        const float card_spacing = Core::GlobalScaling::scaled(40.0f);
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

        const float btn_width = Core::GlobalScaling::scaled(220.0f);
        const float btn_height = Core::GlobalScaling::scaled(60.0f);
        const Rectangle back_rect = { (screen_w - btn_width) / 2.0f, screen_h - Core::GlobalScaling::scaled(140.0f), btn_width, btn_height };
        if (CheckCollisionPointRec(mouse_pos, back_rect)) return "BACK";

        return "";
    }

    void LevelSelectMenu::drawLevelCard(const Rectangle& rect, const World::LevelInfo& info, const bool is_hovered, const Font& font) const {
        // Card background
        DrawRectangleRec(rect, is_hovered ? Fade({ 255, 200, 100, 255 }, 0.2f) : Fade(WHITE, 0.10f));
        DrawRectangleLinesEx(rect, Core::GlobalScaling::scaled(2.0f), is_hovered ? Color{ 255, 200, 100, 255 } : Fade(WHITE, 0.4f));

        const float name_font_size = Core::GlobalScaling::scaled(UIHandler::FONT_SIZE_SUBTITLE);
        const float spacing = Core::GlobalScaling::scaled(1.0f);
        const Vector2 name_size = MeasureTextEx(font, info.name.c_str(), name_font_size, spacing);
        DrawTextEx(font, info.name.c_str(), { rect.x + (rect.width - name_size.x) / 2.0f, rect.y + Core::GlobalScaling::scaled(20.0f) }, name_font_size, spacing, WHITE);

        const float sep_y = rect.y + Core::GlobalScaling::scaled(65.0f);
        DrawLineEx({rect.x + Core::GlobalScaling::scaled(20.0f), sep_y}, {rect.x + rect.width - Core::GlobalScaling::scaled(20.0f), sep_y}, 1.0f, Fade(WHITE, 0.3f));

        const float loc_font_size = Core::GlobalScaling::scaled(UIHandler::FONT_SIZE_TEXT);
        float loc_y = sep_y + Core::GlobalScaling::scaled(15.0f);
        DrawTextEx(font, "Lokacje:", { rect.x + Core::GlobalScaling::scaled(20.0f), loc_y }, loc_font_size, spacing, { 255, 200, 100, 255 });
        loc_y += loc_font_size + Core::GlobalScaling::scaled(8.0f);

        for (const auto& loc : info.locations) {
            std::string bullet = "- " + loc;
            DrawTextEx(font, bullet.c_str(), { rect.x + Core::GlobalScaling::scaled(30.0f), loc_y }, loc_font_size, spacing, Fade(WHITE, 0.8f));
            loc_y += loc_font_size + Core::GlobalScaling::scaled(4.0f);
        }
    }

    void LevelSelectMenu::drawButton(const Rectangle& rect, const char* text, const bool is_hovered, const Font& font) const {
        // Unused now, keeping for compatibility if needed or until fully cleaned up
    }

} // namespace Nawia::UI
