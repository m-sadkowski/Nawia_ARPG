#include "BossManager.h"

#include <BossPreloader.h>
#include <Engine.h>
#include <EnemyInterface.h>
#include <Logger.h>
#include <Map.h>

namespace Nawia::Game {

    void BossManager::clearPreloadedBosses() {
        _minion_pools.clear();
        _boss_pool.clear();
    }

    void BossManager::resetRuntimeState(Core::Engine* engine) {
        removeMinions(engine);

        if (_active_boss_entity)
            _active_boss_entity->setHealToFullOnKill(false);

        if (_active_boss_entity && !_active_boss_entity->isDead())
            _active_boss_entity->die();

        _active_boss_data = nullptr;
        _active_boss_entity = nullptr;
        _current_phase_index = 0;
        _fight_timer = 0.0f;
        _phase_flash_timer = 0.0f;
        _minion_pools.clear();
        _boss_pool.clear();
        restoreMusicAfterBoss(engine);
    }

    void BossManager::preloadBossFight(const std::string& boss_id, Core::Engine* engine) {
        if (boss_id.empty() || !engine || !engine->getCurrentMap())
            return;

        const auto boss_it = _bosses.find(boss_id);
        if (boss_it == _bosses.end()) {
            Core::Logger::errorLog("BossManager: Nie mozna preloadowac nieznanego bossa '" + boss_id + "'.");
            return;
        }

        if (BossPreloader::preloadBossDefinition(boss_it->second, engine, _boss_pool, _minion_pools)) {
            Core::Logger::debugLog("BossManager: Preladowano walke z bossem '" + boss_id + "'.");
        }
    }

    void BossManager::update(Core::Engine* engine, float dt) {
        if (!isFightActive()) return;

        _fight_timer += dt;

        if (_phase_flash_timer > 0.0f) {
            _phase_flash_timer -= dt;
            if (_phase_flash_timer < 0.0f) _phase_flash_timer = 0.0f;
        }

        if (_active_boss_entity && _active_boss_entity->isDead()) {
            endBossFight(true, engine);
            return;
        }

        if (_active_boss_entity && _active_boss_entity->isDying())
            return;

        for (auto it = _active_minions.begin(); it != _active_minions.end();) {
            if (!*it || (*it)->isDead()) {
                it = _active_minions.erase(it);
            } else {
                ++it;
            }
        }

        checkPhaseTransition(engine);
    }

} // namespace Nawia::Game
