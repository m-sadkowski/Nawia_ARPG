#include "StatsUI.h"
#include "../entity/actors/player/Player.h"
#include "../entity/actors/player/Stats.h"

namespace Nawia::UI {
    StatsUI::StatsUI(std::shared_ptr<Entity::Player> player) : _player(player) {}

    void StatsUI::render(float x, float y) const {
        if (!_player) return;

        // Draw background
        DrawRectangle(x, y, 300, 160, Fade(BLACK, 0.7f));
        DrawRectangleLines(x, y, 300, 160, WHITE);

        const auto& stats = _player->getStats();
        // Base entity HP vs Max HP from Stats
        // Currently Entity uses _max_hp. 
        // We should ensure Player's _max_hp is synced with stats.max_hp (I added that in recalculateStats).
        
        int offset = 10;
        DrawText("Player Stats", x + 10, y + offset, 20, GOLD); 
        offset += 30;
        
        DrawText(TextFormat("Health: %d / %d", _player->getHP(), stats.max_hp), x + 10, y + offset, 20, GREEN); offset += 30;
        DrawText(TextFormat("Damage: %d", stats.damage), x + 10, y + offset, 20, RED); offset += 30;
        DrawText(TextFormat("Atk Spd: %.2f", stats.attack_speed), x + 10, y + offset, 20, YELLOW); offset += 30;
        DrawText(TextFormat("Tenacity: %d", stats.tenacity), x + 10, y + offset, 20, BLUE);
    }
}
