#include "LevelSelectMenu.h"
#include "UIHandler.h"

#include <GlobalScaling.h>

namespace Nawia::UI
{

    LevelSelectMenu::LevelSelectMenu(const std::vector<World::LevelInfo>& levels) 
        : _levels(levels)
        , _level_selected(false) 
    {}

    void LevelSelectMenu::render(const UIHandler& ui) const
    {
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());
        const float font_spacing = Core::GlobalScaling::scaled(2.0f);
        
        DrawRectangle(0, 0, static_cast<int>(screen_width), static_cast<int>(screen_height), Fade(BLACK, 0.4f));
        
        const float title_font_size = Core::GlobalScaling::scaled(FONT_SIZE_TITLE); 
        const char* menu_title = LABEL_SELECT_LEVEL; 
        const Vector2 title_size = MeasureTextEx(ui.getFont(), menu_title, title_font_size, font_spacing);
        
        DrawTextEx(ui.getFont(), menu_title, {(screen_width - title_size.x) / 2.0f, Core::GlobalScaling::scaled(60.0f)}, title_font_size, font_spacing, COLOR_ACCENT);
        
        const float card_width = Core::GlobalScaling::scaled(280.0f);
        const float card_height = Core::GlobalScaling::scaled(200.0f);
        const float card_spacing = Core::GlobalScaling::scaled(40.0f);
        const float total_cards_width = _levels.size() * card_width + (_levels.size() - 1) * card_spacing;
        
        const float start_x = (screen_width - total_cards_width) / 2.0f; 
        const float start_y = screen_height / 2.0f - card_height / 2.0f;
        
        for (size_t i = 0; i < _levels.size(); ++i)
        { 
            const Rectangle card_rect = { start_x + i * (card_width + card_spacing), start_y, card_width, card_height }; 
            drawLevelCard(card_rect, _levels[i], CheckCollisionPointRec(GetMousePosition(), card_rect), ui.getFont()); 
        }
        
        const float button_width = Core::GlobalScaling::scaled(BUTTON_WIDTH * 0.65f);
        const float button_height = Core::GlobalScaling::scaled(BUTTON_HEIGHT * 0.85f);
        const float bottom_offset = Core::GlobalScaling::scaled(BACK_BUTTON_BOTTOM_OFFSET);
        
        const Rectangle back_button_rect = { (screen_width - button_width) / 2.0f, screen_height - bottom_offset, button_width, button_height };
        ui.drawMenuButton(back_button_rect, LABEL_BACK, CheckCollisionPointRec(GetMousePosition(), back_button_rect) ? 1.0f : 0.0f);
    }

    std::string LevelSelectMenu::handleInput()
    {
        if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            return "";
        
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());
        
        const float card_width = Core::GlobalScaling::scaled(280.0f);
        const float card_height = Core::GlobalScaling::scaled(200.0f);
        const float card_spacing = Core::GlobalScaling::scaled(40.0f);
        const float total_cards_width = _levels.size() * card_width + (_levels.size() - 1) * card_spacing;
        
        const float start_x = (screen_width - total_cards_width) / 2.0f; 
        const float start_y = screen_height / 2.0f - card_height / 2.0f; 
        const Vector2 mouse_pos = GetMousePosition();
        
        for (size_t i = 0; i < _levels.size(); ++i)
        { 
            const Rectangle card_rect = { start_x + i * (card_width + card_spacing), start_y, card_width, card_height };
            if (CheckCollisionPointRec(mouse_pos, card_rect))
            { 
                _level_selected = true; 
                _selected_level_name = _levels[i].name; 
                return _levels[i].name; 
            } 
        }
        
        const float button_width = Core::GlobalScaling::scaled(BUTTON_WIDTH * 0.65f);
        const float button_height = Core::GlobalScaling::scaled(BUTTON_HEIGHT * 0.85f);
        const float bottom_offset = Core::GlobalScaling::scaled(BACK_BUTTON_BOTTOM_OFFSET);
        
        if (CheckCollisionPointRec(mouse_pos, { (screen_width - button_width) / 2.0f, screen_height - bottom_offset, button_width, button_height }))
            return "BACK";
        
        return "";
    }

    void LevelSelectMenu::drawLevelCard(const Rectangle& card_rect, const World::LevelInfo& info, bool is_hovered, const Font& font) const
    {
        DrawRectangleRec(card_rect, is_hovered ? COLOR_ACCENT_SOFT : Fade(WHITE, 0.10f)); 
        DrawRectangleLinesEx(card_rect, Core::GlobalScaling::scaled(2.0f), is_hovered ? COLOR_ACCENT : Fade(WHITE, 0.4f));
        
        const float subtitle_font_size = Core::GlobalScaling::scaled(FONT_SIZE_SUBTITLE);
        const float font_spacing = Core::GlobalScaling::scaled(1.0f); 
        const Vector2 name_size = MeasureTextEx(font, info.name.c_str(), subtitle_font_size, font_spacing);
        
        DrawTextEx(font, info.name.c_str(), { card_rect.x + (card_rect.width - name_size.x) / 2.0f, card_rect.y + Core::GlobalScaling::scaled(20.0f) }, subtitle_font_size, font_spacing, WHITE);
        
        const float separator_y = card_rect.y + Core::GlobalScaling::scaled(65.0f); 
        DrawLineEx({card_rect.x + Core::GlobalScaling::scaled(20.0f), separator_y}, {card_rect.x + card_rect.width - Core::GlobalScaling::scaled(20.0f), separator_y}, 1.0f, Fade(WHITE, 0.3f));
        
        const float text_font_size = Core::GlobalScaling::scaled(FONT_SIZE_TEXT); 
        float location_y = separator_y + Core::GlobalScaling::scaled(15.0f);
        
        DrawTextEx(font, "Lokacje:", { card_rect.x + Core::GlobalScaling::scaled(20.0f), location_y }, text_font_size, font_spacing, COLOR_ACCENT); 
        location_y += text_font_size + Core::GlobalScaling::scaled(8.0f);
        
        for (const auto& location : info.locations)
        { 
            const std::string bullet_point = "- " + location; 
            DrawTextEx(font, bullet_point.c_str(), { card_rect.x + Core::GlobalScaling::scaled(30.0f), location_y }, text_font_size, font_spacing, Fade(WHITE, 0.8f)); 
            location_y += text_font_size + Core::GlobalScaling::scaled(4.0f); 
        }
    }
} // namespace Nawia::UI
