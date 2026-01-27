#include "StatsUI.h"
#include "../entity/actors/player/Player.h"
#include "../entity/actors/player/Stats.h"
#include <GlobalScaling.h>

namespace Nawia::UI {
    StatsUI::StatsUI(std::shared_ptr<Entity::Player> player) : _player(player) {}

    void StatsUI::render(float x, float y, const Font& font) const {
        if (!_player) return;

        // Use global scaling for dimensions
        const float width = Core::GlobalScaling::scaled(300.0f);
        const float height = Core::GlobalScaling::scaled(240.0f); // Slightly taller for better spacing
        
        // Draw background (Menu Style: Semi-transparent Black)
        DrawRectangle(static_cast<int>(x), static_cast<int>(y), static_cast<int>(width), static_cast<int>(height), Fade(BLACK, 0.8f));
        
        // Draw border (Menu Style: White/LightGray outline)
        DrawRectangleLinesEx({ x, y, width, height }, Core::GlobalScaling::scaled(2.0f), DARKGRAY);

        const auto& stats = _player->getStats();
        
        // Font setup
        const float title_font_size = Core::GlobalScaling::scaled(24.0f);
        const float text_font_size = Core::GlobalScaling::scaled(18.0f);
        const float spacing = Core::GlobalScaling::scaled(1.0f);
        float current_y = y + Core::GlobalScaling::scaled(15.0f);
        const float x_padding = Core::GlobalScaling::scaled(15.0f);
        const float line_height = Core::GlobalScaling::scaled(30.0f);

        // Title
        DrawTextEx(font, "PLAYER STATS", { x + x_padding, current_y }, title_font_size, spacing, GOLD);
        current_y += line_height * 1.2f; // Extra space after title
        
        // Helper lambda for consistent rows
        auto drawStatRow = [&](const char* label, const char* value, Color color) {
            DrawTextEx(font, label, { x + x_padding, current_y }, text_font_size, spacing, LIGHTGRAY);
            
            Vector2 valSize = MeasureTextEx(font, value, text_font_size, spacing);
            DrawTextEx(font, value, { x + width - x_padding - valSize.x, current_y }, text_font_size, spacing, color);
            
            current_y += line_height;
        };

        drawStatRow("Health", TextFormat("%d / %d", _player->getHP(), stats.max_hp), RED);
        drawStatRow("Damage", TextFormat("%d", stats.damage), ORANGE);
        drawStatRow("Atk Speed", TextFormat("%.2f", stats.attack_speed), YELLOW);
        drawStatRow("Move Speed", TextFormat("%.2f", stats.movement_speed), SKYBLUE);
        drawStatRow("Tenacity", TextFormat("%d", stats.tenacity), BLUE);
    }
}
