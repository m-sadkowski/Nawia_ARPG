#include "BossManager.h"
#include "BossManagerInternal.h"

#include <BossEnemyFactory.h>
#include <BossPhaseMath.h>
#include <BossRuntimeSerializer.h>
#include <Engine.h>
#include <Entity.h>
#include <EnemyInterface.h>
#include <Logger.h>

#include <algorithm>

namespace Nawia::Game {

    BossRuntimeState BossManager::getRuntimeState() const {
        BossRuntimeState state;
        state.active = isFightActive() && _active_boss_data && _active_boss_entity;
        if (!state.active)
            return state;

        state.boss_id = _active_boss_data->id;
        state.current_phase_index = BossPhaseMath::resolvePhaseIndexForHp(
            *_active_boss_data,
            _current_phase_index,
            _active_boss_entity->getHP(),
            _active_boss_entity->getMaxHP()
        );
        state.fight_timer = _fight_timer;
        state.saved_hp = _active_boss_entity->getHP();
        state.max_hp = _active_boss_entity->getMaxHP();
        state.position = {_active_boss_entity->getX(), _active_boss_entity->getY()};
        state.altitude = _active_boss_entity->getAltitude();
        state.spawn_position = _active_boss_spawn_pos;
        state.spawn_altitude = _active_boss_spawn_altitude;
        return state;
    }

    nlohmann::json BossManager::serializeRuntimeState() const {
        return BossRuntimeSerializer::toJson(getRuntimeState());
    }

    bool BossManager::applyRuntimeState(const nlohmann::json& data, Core::Engine* engine) {
        BossRuntimeState state;
        if (!BossRuntimeSerializer::fromJson(data, state))
            return false;

        return restoreRuntimeState(state, engine);
    }

    bool BossManager::restoreRuntimeState(const BossRuntimeState& state, Core::Engine* engine) {
        if (!state.active || state.boss_id.empty() || !engine)
            return false;

        if (isBossDefeated(state.boss_id))
            return false;

        if (isFightActive())
            resetRuntimeState(engine);

        const auto boss_it = _bosses.find(state.boss_id);
        if (boss_it == _bosses.end()) {
            Core::Logger::errorLog("BossManager: Nie mozna odtworzyc nieznanego bossa '" + state.boss_id + "'.");
            return false;
        }

        _active_boss_data = &boss_it->second;
        _active_boss_spawn_pos = state.spawn_position;
        _active_boss_spawn_altitude = state.spawn_altitude;
        const int max_phase_index = _active_boss_data->phases.empty()
            ? 0
            : static_cast<int>(_active_boss_data->phases.size()) - 1;
        _current_phase_index = std::clamp(
            state.current_phase_index,
            0,
            max_phase_index
        );
        _fight_timer = std::max(0.0f, state.fight_timer);
        _phase_flash_timer = 0.0f;

        if (!activateBossFromPool(state.boss_id, engine) && !buildAndActivateBoss(engine)) {
            _active_boss_data = nullptr;
            return false;
        }

        if (!_active_boss_entity) {
            _active_boss_data = nullptr;
            return false;
        }

        const int max_hp = state.max_hp > 0 ? state.max_hp : _active_boss_data->max_hp;
        _active_boss_entity->setMaxHp(max_hp);
        _active_boss_entity->setX(state.position.x);
        _active_boss_entity->setY(state.position.y);
        _active_boss_entity->setAltitude(state.altitude);
        _active_boss_entity->setDormant(false);

        const int restart_hp = BossPhaseMath::restartHp(*_active_boss_data, _current_phase_index, max_hp);
        _active_boss_entity->setHP(restart_hp);

        if (!_active_boss_data->phases.empty())
            applyPhase(_active_boss_data->phases[_current_phase_index], engine);

        startBossMusic(engine);

        Core::Logger::debugLog("BossManager: Odtworzono walke z bossem '" + state.boss_id +
            "' w fazie " + std::to_string(_current_phase_index + 1) +
            " z HP " + std::to_string(restart_hp) + ".");
        return true;
    }

    std::shared_ptr<Entity::Entity> BossManager::createPreviewEntity(const BossData& boss_data, Core::Engine* engine) {
        if (!engine)
            return nullptr;

        auto entity = BossEnemyFactory::create(
            boss_data.enemy_type,
            boss_data.name.empty() ? boss_data.id : boss_data.name,
            boss_data.max_hp,
            engine);
        if (!entity)
            return nullptr;

        entity->setType(Entity::EntityType::NPCStatic);
        entity->setFaction(Entity::Faction::None);
        entity->setTarget(nullptr);
        BossManagerDetail::applyConfiguredScale(entity, boss_data.scale);
        return entity;
    }

} // namespace Nawia::Game
