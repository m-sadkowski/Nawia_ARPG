#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <raylib.h>

namespace Nawia::Entity {
    class EnemyInterface;
    class Entity;
}

namespace Nawia::Core {
    class Engine;
}

namespace Nawia::Game {

    struct BossReward {
        std::vector<int> item_ids;
        int gold = 0;
        int exp = 0;
    };

    struct MinionSpawnInfo {
        std::string enemy_type;   // "WalkingDead", "Bandit", "Devil"
        int count = 1;
        int hp = 60;
        float offset_x = 3.0f;   // Spawn offset from boss
        float offset_y = 2.0f;
    };

    struct BossPhase {
        float hp_threshold = 1.0f;   // Phase triggers when HP% drops below this (1.0 = 100%)
        std::string name;
        float speed_multiplier = 1.0f;
        float damage_multiplier = 1.0f;
        std::string notification;
        std::vector<MinionSpawnInfo> minions;
        bool screen_flash = false;          // Flash screen on phase enter
        Color flash_color = { 255, 0, 0, 180 }; // Flash color
    };

    struct BossData {
        std::string id;
        std::string name;
        std::string enemy_type; // e.g., "Devil"
        int max_hp = 1000;
        float scale = 1.0f;
        
        Vector2 spawn_pos;
        
        std::vector<BossPhase> phases;
        BossReward reward;
        std::string level_name;
        
        // "end_fight" = walls drop, fight ends; "retry" = player respawns and can retry
        std::string on_player_death = "end_fight";
    };

    class BossManager {
    public:
        BossManager();
        ~BossManager();

        void loadFromJson(const std::string& path);
        void preloadForLevel(const std::string& level_name, Core::Engine* engine);
        void update(Core::Engine* engine, float dt);
        
        bool startBossFight(const std::string& boss_id, Core::Engine* engine);
        void endBossFight(bool victory, Core::Engine* engine);

        [[nodiscard]] bool isFightActive() const { return _active_boss_data != nullptr; }
        [[nodiscard]] const BossData* getActiveBossData() const { return _active_boss_data; }
        [[nodiscard]] std::shared_ptr<Entity::EnemyInterface> getActiveBossEntity() const { return _active_boss_entity; }
        [[nodiscard]] int getCurrentPhaseIndex() const { return _current_phase_index; }
        [[nodiscard]] float getFightTimer() const { return _fight_timer; }
        [[nodiscard]] bool isBossDefeated(const std::string& boss_id) const { return _defeated_bosses.count(boss_id) > 0; }

        [[nodiscard]] const std::map<std::string, BossData>& getAllBosses() const { return _bosses; }

        /// Phase transition screen flash effect — returns remaining flash time (0 = no flash)
        [[nodiscard]] float getPhaseFlashTimer() const { return _phase_flash_timer; }
        [[nodiscard]] Color getPhaseFlashColor() const { return _phase_flash_color; }

    private:
        std::map<std::string, BossData> _bosses;
        
        const BossData* _active_boss_data = nullptr;
        std::shared_ptr<Entity::EnemyInterface> _active_boss_entity = nullptr;
        
        // Phase system
        int _current_phase_index = 0;
        float _fight_timer = 0.0f;
        
        // Phase transition flash effect
        float _phase_flash_timer = 0.0f;
        Color _phase_flash_color = { 255, 0, 0, 180 };
        
        void checkPhaseTransition(Core::Engine* engine);
        void applyPhase(const BossPhase& phase, Core::Engine* engine);
        
        // Minions
        void spawnMinions(const std::vector<MinionSpawnInfo>& minions, Core::Engine* engine);
        void removeMinions(Core::Engine* engine);
        std::vector<std::shared_ptr<Entity::Entity>> _active_minions;
        std::map<std::string, std::vector<std::shared_ptr<Entity::Entity>>> _minion_pools;
        std::map<std::string, std::shared_ptr<Entity::Entity>> _boss_pool;
        
        // Track defeated bosses so fights can't be re-triggered
        std::set<std::string> _defeated_bosses;
    };

} // namespace Nawia::Game
