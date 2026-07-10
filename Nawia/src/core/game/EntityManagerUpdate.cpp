#include "EntityManager.h"

#include <AbilityEffect.h>
#include <Engine.h>
#include <Entity.h>
#include <EntityNavigationSupport.h>
#include <Map.h>
#include <QuestManager.h>

#include <algorithm>
#include <limits>

namespace Nawia::Core {

    namespace {

        constexpr float k_combat_target_refresh_interval = 0.25f;
        constexpr float k_altitude_snap_interval = 0.10f;

        bool usesNavMeshAltitude(const std::shared_ptr<Entity::Entity>& entity) {
            if (!entity || entity->isDead() || entity->isDormant())
                return false;

            const Entity::EntityType type = entity->getType();
            return type == Entity::EntityType::Player ||
                   type == Entity::EntityType::Enemy ||
                   type == Entity::EntityType::Ally ||
                   type == Entity::EntityType::NPCStatic ||
                   type == Entity::EntityType::NPCActor;
        }

        bool isValidCombatTarget(
            const Entity::EntityType seeker_type,
            const std::shared_ptr<Entity::Entity>& candidate)
        {
            if (!candidate || candidate->isDead() || candidate->isDying() || candidate->isDormant())
                return false;

            const Entity::EntityType candidate_type = candidate->getType();
            if (seeker_type == Entity::EntityType::Enemy)
                return candidate_type == Entity::EntityType::Player || candidate_type == Entity::EntityType::Ally;
            if (seeker_type == Entity::EntityType::Ally)
                return candidate_type == Entity::EntityType::Enemy;

            return false;
        }

    } // namespace

    void EntityManager::updateEntities(const float delta_time) {
        _combat_target_refresh_timer -= delta_time;
        if (_combat_target_refresh_timer <= 0.0f) {
            refreshCombatTargets();
            _combat_target_refresh_timer = k_combat_target_refresh_interval;
        }

        _altitude_snap_timer -= delta_time;
        const bool should_snap_altitudes = _altitude_snap_timer <= 0.0f;
        if (should_snap_altitudes)
            _altitude_snap_timer = k_altitude_snap_interval;

        for (auto it = _active_entities.begin(); it != _active_entities.end();) {
            const auto& entity = *it;
            entity->update(delta_time);

            if (should_snap_altitudes && _engine && usesNavMeshAltitude(entity))
                Entity::EntityNavigationSupport::snapAltitudeToNavmesh(*entity, _engine->getCurrentMap());

            bool is_expired_spell = false;
            if (const auto ability_effect = dynamic_cast<Entity::AbilityEffect*>(entity.get()))
                is_expired_spell = ability_effect->isExpired();

            if (entity->isDead() || is_expired_spell) {
                if (entity->isDead() && entity->shouldPersistAfterDeath()) {
                    ++it;
                    continue;
                }

                if (entity->isDead() && entity->getType() == Entity::EntityType::Enemy && _engine)
                    _engine->getQuestManager().notifyKill(entity->getName());

                it = _active_entities.erase(it);
            } else {
                ++it;
            }
        }
    }

    void EntityManager::refreshCombatTargets() {
        for (const auto& entity : _active_entities) {
            if (!entity || entity->isDead() || entity->isDying() || entity->isDormant())
                continue;

            const Entity::EntityType type = entity->getType();
            if (type != Entity::EntityType::Enemy && type != Entity::EntityType::Ally)
                continue;

            entity->setTarget(findClosestCombatTarget(entity));
        }
    }

    std::shared_ptr<Entity::Entity> EntityManager::findClosestCombatTarget(
        const std::shared_ptr<Entity::Entity>& seeker) const
    {
        if (!seeker)
            return nullptr;

        const Entity::EntityType seeker_type = seeker->getType();
        float best_distance_sq = std::numeric_limits<float>::max();
        std::shared_ptr<Entity::Entity> best_target = nullptr;

        if (const auto damage_source = seeker->getLastDamageSource(); isValidCombatTarget(seeker_type, damage_source))
            return damage_source;

        for (const auto& candidate : _active_entities) {
            if (!candidate || candidate == seeker)
                continue;

            if (!isValidCombatTarget(seeker_type, candidate))
                continue;

            const float dx = seeker->getCenter().x - candidate->getCenter().x;
            const float dy = seeker->getCenter().y - candidate->getCenter().y;
            const float distance_sq = dx * dx + dy * dy;

            if (distance_sq < best_distance_sq) {
                best_distance_sq = distance_sq;
                best_target = candidate;
            }
        }

        return best_target;
    }

} // namespace Nawia::Core
