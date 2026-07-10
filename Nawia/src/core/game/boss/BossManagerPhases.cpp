#include "BossManager.h"

#include <BossMinionSpawner.h>
#include <Engine.h>
#include <EnemyInterface.h>
#include <Logger.h>
#include <UIHandler.h>

namespace Nawia::Game {

    void BossManager::checkPhaseTransition(Core::Engine* engine) {
        if (!_active_boss_entity || !_active_boss_data) return;
        if (_active_boss_data->phases.empty()) return;

        float hp_pct = static_cast<float>(_active_boss_entity->getHP()) /
                       static_cast<float>(_active_boss_entity->getMaxHP());

        for (int i = static_cast<int>(_active_boss_data->phases.size()) - 1; i > _current_phase_index; --i) {
            if (hp_pct <= _active_boss_data->phases[i].hp_threshold) {
                _current_phase_index = i;
                applyPhase(_active_boss_data->phases[i], engine);
                break;
            }
        }
    }

    void BossManager::applyPhase(const BossPhase& phase, Core::Engine* engine) {
        Core::Logger::debugLog("BossManager: Wejscie w faze: " + phase.name);

        if (_active_boss_entity) {
            _active_boss_entity->setSpeedMultiplier(phase.speed_multiplier);
            _active_boss_entity->setDamageMultiplier(phase.damage_multiplier);
        }

        if (!phase.notification.empty()) {
            engine->getUIHandler().showNotification(phase.notification, 3.0f);
        }

        if (phase.screen_flash) {
            _phase_flash_timer = 0.6f;
            _phase_flash_color = phase.flash_color;
        }

        if (!phase.minions.empty()) {
            spawnMinions(phase.minions, engine);
        }
    }

    void BossManager::spawnMinions(const std::vector<MinionSpawnInfo>& minions, Core::Engine* engine) {
        BossMinionSpawner::spawn(minions, _active_boss_entity, _minion_pools, _active_minions, engine);
    }

    void BossManager::removeMinions(Core::Engine* engine) {
        (void)engine;
        BossMinionSpawner::remove(_active_minions);
    }

} // namespace Nawia::Game
