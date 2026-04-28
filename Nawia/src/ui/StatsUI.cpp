#include "StatsUI.h"
#include <Player.h>
#include <Stats.h>
#include <GlobalScaling.h>
#include <UIDefines.h>
#include <UIRenderUtils.h>

namespace Nawia::UI
{

    StatsUI::StatsUI(const std::shared_ptr<Entity::Player>& player) : _player(player) {}

    void StatsUI::render(float x, float y, const Font& font) const
    {
        if (!_player)
            return;

        const float width = Core::GlobalScaling::scaled(280.0f);
        const float height = Core::GlobalScaling::scaled(220.0f);
        
        // AAA Premium Panel
        DrawRectangleRec({ x, y, width, height }, withAlpha(COLOR_PANEL_BG, 0.95f));
        DrawRectangleLinesEx({ x, y, width, height }, 1.5f, withAlpha(COLOR_ACCENT, 0.8f));
        
        const auto& stats = _player->getStats();
        
        const float title_font_size = Core::GlobalScaling::scaled(22.0f);
        const float text_font_size = Core::GlobalScaling::scaled(18.0f);
        const float spacing = Core::GlobalScaling::scaled(1.0f);
        
        float current_y = y + Core::GlobalScaling::scaled(15.0f);
        const float x_padding = Core::GlobalScaling::scaled(15.0f);
        const float line_height = Core::GlobalScaling::scaled(32.0f);

        // Title
        DrawTextEx(font, "STATYSTYKI", { x + x_padding, current_y }, title_font_size, spacing, COLOR_ACCENT);
        current_y += line_height * 1.1f;
        
        auto draw_stat_row = [&](const char* label, const char* value, Color value_color)
        {
            DrawTextEx(font, label, { x + x_padding, current_y }, text_font_size, spacing, withAlpha(COLOR_PARCHMENT, 0.7f));
            
            const Vector2 value_size = MeasureTextEx(font, value, text_font_size, spacing);
            DrawTextEx(font, value, { x + width - x_padding - value_size.x, current_y }, text_font_size, spacing, value_color);
            
            // Subtle horizontal separator
            DrawLineEx({ x + x_padding, current_y + text_font_size + 4.0f }, { x + width - x_padding, current_y + text_font_size + 4.0f }, 1.0f, withAlpha(WHITE, 0.05f));
            
            current_y += line_height;
        };
        
        draw_stat_row("ZDROWIE", TextFormat("%d / %d", _player->getHP(), stats.max_hp), RED);
        draw_stat_row("SILA", TextFormat("%d", stats.damage), COLOR_SLAVIC_ORANGE);
        draw_stat_row("PREDKOSC ATAKU", TextFormat("%.2f", stats.attack_speed), COLOR_GOLDEN_TEXT);
        draw_stat_row("PREDKOSC", TextFormat("%.2f", stats.movement_speed), COLOR_SLAVIC_BLUE);
        draw_stat_row("NIEUSTEPLIWOSC", TextFormat("%d", stats.tenacity), DARKGRAY);
    }
} // namespace Nawia::UI
