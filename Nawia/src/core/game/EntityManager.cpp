#include "EntityManager.h"

#include <Engine.h>
#include <Logger.h>
#include <Map.h>

#include <AbilityEffect.h>
#include <Collider.h>
#include <Entity.h>
#include <InteractiveTrigger.h>

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace Nawia::Core {

    namespace {

        constexpr float k_combat_target_refresh_interval = 0.25f;
        constexpr float k_altitude_snap_interval = 0.10f;
        constexpr float k_physical_collision_radius = 0.8f;

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

        bool isAbilityTarget(const std::shared_ptr<Entity::Entity>& entity) {
            if (!entity || entity->isDormant())
                return false;

            const Entity::EntityType type = entity->getType();
            return type != Entity::EntityType::Projectile &&
                   type != Entity::EntityType::Chest &&
                   type != Entity::EntityType::Trigger &&
                   type != Entity::EntityType::NPCStatic &&
                   type != Entity::EntityType::NPCActor;
        }

        bool isValidCombatTarget(
            const Entity::EntityType seeker_type,
            const std::shared_ptr<Entity::Entity>& candidate) {
            if (!candidate || candidate->isDead() || candidate->isDying() || candidate->isDormant())
                return false;

            const Entity::EntityType candidate_type = candidate->getType();
            if (seeker_type == Entity::EntityType::Enemy)
                return candidate_type == Entity::EntityType::Player || candidate_type == Entity::EntityType::Ally;
            if (seeker_type == Entity::EntityType::Ally)
                return candidate_type == Entity::EntityType::Enemy;

            return false;
        }

    }

    void EntityManager::addEntity(std::shared_ptr<Entity::Entity> new_entity) {
        if (!new_entity)
            return;

        if (std::ranges::find(_active_entities, new_entity) != _active_entities.end())
            return;

        _active_entities.push_back(std::move(new_entity));
    }

    void EntityManager::setPlayer(std::shared_ptr<Entity::Entity> player) {
        _player = std::move(player);
        Entity::Entity::setAudioListener(_player);
    }

	void EntityManager::clearNonPlayerEntities() {
		std::vector<std::shared_ptr<Entity::Entity>> retained_entities;
		if (_player) 
        {
			retained_entities.push_back(_player);
		} 
    	else 
    	{
			for (const auto& entity : _active_entities) {
				if (entity->getName() == "Player") {
					retained_entities.push_back(entity);
					break;
				}
			}
		}
		_active_entities = std::move(retained_entities);
        _combat_target_refresh_timer = 0.0f;
        _altitude_snap_timer = 0.0f;
	}

    std::shared_ptr<Entity::Entity> EntityManager::getEntityAt(const float screen_x, const float screen_y, const Camera3D& camera) const {
        // Iterujemy od konca, zeby najpierw lapac encje narysowane najwyzej.
        for (auto it = _active_entities.rbegin(); it != _active_entities.rend(); ++it) {
            if ((*it)->isDormant()) continue;
            if ((*it)->isMouseOver(screen_x, screen_y, camera))
                return *it;
        }
        return nullptr;
    }

    void EntityManager::updateHoverState(const float screen_x, const float screen_y, const Camera3D& camera) {
        for (const auto& entity : _active_entities)
            entity->setHovered(false);

        for (auto it = _active_entities.rbegin(); it != _active_entities.rend(); ++it) 
        {
            if ((*it)->isDormant()) continue;
            if ((*it)->isMouseOver(screen_x, screen_y, camera)) {
                (*it)->setHovered(true);
                return;
            }
        }
    }

    void EntityManager::renderEntities(const Camera3D& camera) const {
        std::vector<Entity::Entity*> render_list;
        render_list.reserve(_active_entities.size());

        for (const auto& entity : _active_entities)
        {
            if (!entity->isDormant() && entity->isVisibleInCamera(camera))
                render_list.push_back(entity.get());
        }

		// Sortowanie po Y trzyma poprawna kolejnosc nakladania modeli.
        std::ranges::sort(render_list, {}, &Entity::Entity::getY);

        for (auto* entity : render_list) {
            entity->render(camera);
        }
    }

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

        for (auto it = _active_entities.begin(); it != _active_entities.end();)
        {
            const auto& entity = *it;
            entity->update(delta_time);

            // Dociaga wysokosc encji do navmesha aktualnej mapy.
            if (should_snap_altitudes && _engine && _engine->getCurrentMap() && usesNavMeshAltitude(entity)) {
                const Vector3 current_position = { entity->getX(), entity->getAltitude(), entity->getY() };
                const Vector3 snapped_position = _engine->getCurrentMap()->getNavMesh().getClosestWalkablePosition(current_position);
                entity->setAltitude(snapped_position.y);
            }

            // Efekty umiejetnosci usuwamy po wygasnieciu.
            bool is_expired_spell = false;
            if (const auto ability_effect = dynamic_cast<Entity::AbilityEffect*>(entity.get()))
                is_expired_spell = ability_effect->isExpired();

			if (entity->isDead() || is_expired_spell) {
				if (entity->isDead() && entity->shouldPersistAfterDeath()) {
					++it;
					continue;
				}

				if (entity->isDead() && entity->getType() == Entity::EntityType::Enemy) {
					if (_engine) {
						_engine->getQuestManager().notifyKill(entity->getName());
                    }
                }
                
                it = _active_entities.erase(it);
            }
                
            else
                ++it;
        }
    }

    void EntityManager::refreshCombatTargets()
    {
        for (const auto& entity : _active_entities)
        {
            if (!entity || entity->isDead() || entity->isDying() || entity->isDormant())
                continue;

            const Entity::EntityType type = entity->getType();
            if (type != Entity::EntityType::Enemy && type != Entity::EntityType::Ally)
                continue;

            entity->setTarget(findClosestCombatTarget(entity));
        }
    }

    std::shared_ptr<Entity::Entity> EntityManager::findClosestCombatTarget(const std::shared_ptr<Entity::Entity>& seeker) const
    {
        if (!seeker) return nullptr;

        const Entity::EntityType seeker_type = seeker->getType();
        float best_distance_sq = std::numeric_limits<float>::max();
        std::shared_ptr<Entity::Entity> best_target = nullptr;

        if (const auto damage_source = seeker->getLastDamageSource(); isValidCombatTarget(seeker_type, damage_source))
            return damage_source;

        for (const auto& candidate : _active_entities)
        {
            if (!candidate || candidate == seeker)
                continue;

            if (!isValidCombatTarget(seeker_type, candidate))
                continue;

            const float dx = seeker->getCenter().x - candidate->getCenter().x;
            const float dy = seeker->getCenter().y - candidate->getCenter().y;
            const float distance_sq = dx * dx + dy * dy;

            if (distance_sq < best_distance_sq)
            {
                best_distance_sq = distance_sq;
                best_target = candidate;
            }
        }

        return best_target;
    }

    // --- Collision System ---

    void EntityManager::handleEntitiesCollisions() const {
        processAbilityCollisions();
        processTriggerCollisions();
        processPhysicalCollisions();
    }

    void EntityManager::processAbilityCollisions() const
    {
        std::vector<std::shared_ptr<Entity::Entity>> targets;
        targets.reserve(_active_entities.size());
        for (const auto& entity : _active_entities) {
            if (isAbilityTarget(entity))
                targets.push_back(entity);
        }

        for (auto& entity1 : _active_entities)
        {
            if (entity1->getType() != Entity::EntityType::Projectile) continue;
            if (entity1->isDormant()) continue;

            auto ability = std::static_pointer_cast<Entity::AbilityEffect>(entity1);
            if (ability->isExpired()) continue;

            for (auto& entity2 : targets)
            {
                if (entity1 == entity2) continue;

                if (ability->checkCollision(entity2)) {
                    ability->onCollision(entity2);
                }
            }
        }
    }

    void EntityManager::processTriggerCollisions() const
    {
        if (!_player || _player->isDead()) return;

        const auto entities_snapshot = _active_entities;
        for (auto& entity : entities_snapshot)
        {
            if (!entity || entity == _player) continue;
            if (entity->isDormant()) continue;

            if (const auto trigger = dynamic_cast<Entity::InteractiveTrigger*>(entity.get())) 
            {
                if (trigger->getCollider())
                {
                    bool collision = false;
                    if (_player->getCollider()) {
                        collision = trigger->getCollider()->checkCollision(_player->getCollider());
                    } else {
                        collision = trigger->getCollider()->checkCollision(_player->getBoundingBox());
                    }

                    if (collision) {
                        trigger->onTriggerEnter(*_player);
                        return;
                    }
                }
            }
        }
    }

    void EntityManager::processPhysicalCollisions() const
    {
        std::vector<std::shared_ptr<Entity::Entity>> collidable_entities;
        collidable_entities.reserve(_active_entities.size());
        for (const auto& entity : _active_entities) {
            if (isCollidablePhysicalEntity(entity))
                collidable_entities.push_back(entity);
        }

        std::ranges::sort(collidable_entities, [](const auto& left, const auto& right) {
            return left->getX() < right->getX();
        });

        for (size_t i = 0; i < collidable_entities.size(); ++i)
        {
            auto& e1 = collidable_entities[i];

            for (size_t j = i + 1; j < collidable_entities.size(); ++j)
            {
                auto& e2 = collidable_entities[j];
                if (e2->getX() - e1->getX() > k_physical_collision_radius)
                    break;

                resolveOverlap(e1, e2);
            }
        }
    }

    bool EntityManager::isCollidablePhysicalEntity(const std::shared_ptr<Entity::Entity>& entity) const {
        if (!entity) return false;
        if (entity->isDead()) return false;
        if (entity->isDormant()) return false;

        const Entity::EntityType type = entity->getType();
        return type == Entity::EntityType::Player ||
               type == Entity::EntityType::Enemy ||
               type == Entity::EntityType::Ally ||
               type == Entity::EntityType::NPCActor;
    }

    void EntityManager::resolveOverlap(
        const std::shared_ptr<Entity::Entity>& first_entity,
        const std::shared_ptr<Entity::Entity>& second_entity
    ) const {
        // Prosta kolizja radialna blokuje przenikanie postaci bez laczenia z hitboxami ataku.
        const float dx = second_entity->getX() - first_entity->getX();
        const float dy = second_entity->getY() - first_entity->getY();
        const float distance_sq = dx * dx + dy * dy;
        
        // Przyblizony promien fizyczny postaci wynosi 0.4.
        const float combined_radius = k_physical_collision_radius;
        
        if (distance_sq < combined_radius * combined_radius && distance_sq > 0.0001f) {
            const float distance = std::sqrt(distance_sq);
            const float overlap = combined_radius - distance;
            const bool first_rooted = first_entity->isMovementRooted();
            const bool second_rooted = second_entity->isMovementRooted();
            const float first_push_share = first_rooted && !second_rooted ? 0.0f : (second_rooted && !first_rooted ? 1.0f : 0.5f);
            const float second_push_share = second_rooted && !first_rooted ? 0.0f : (first_rooted && !second_rooted ? 1.0f : 0.5f);
            const float push_x = (dx / distance) * overlap;
            const float push_y = (dy / distance) * overlap;

            first_entity->setX(first_entity->getX() - push_x * first_push_share);
            first_entity->setY(first_entity->getY() - push_y * first_push_share);
            second_entity->setX(second_entity->getX() + push_x * second_push_share);
            second_entity->setY(second_entity->getY() + push_y * second_push_share);
        }
    }

} // namespace Nawia::Core
