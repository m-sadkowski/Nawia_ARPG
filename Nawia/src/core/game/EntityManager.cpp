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

    void EntityManager::addEntity(std::shared_ptr<Entity::Entity> new_entity) {
        _active_entities.push_back(std::move(new_entity));
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
            if (!entity->isDormant())
                render_list.push_back(entity.get());
        }

		// Sortowanie po Y trzyma poprawna kolejnosc nakladania modeli.
        std::ranges::sort(render_list, {}, &Entity::Entity::getY);

        for (auto* entity : render_list) {
            entity->render(camera);
        }
    }

    void EntityManager::updateEntities(const float delta_time) {
        refreshCombatTargets();

        for (auto it = _active_entities.begin(); it != _active_entities.end();)
        {
            const auto& entity = *it;
            entity->update(delta_time);

            // Dociaga wysokosc encji do navmesha aktualnej mapy.
            if (_engine && _engine->getCurrentMap()) {
                const Vector3 current_position = { entity->getX(), entity->getAltitude(), entity->getY() };
                const Vector3 snapped_position = _engine->getCurrentMap()->getNavMesh().getClosestWalkablePosition(current_position);
                entity->setAltitude(snapped_position.y);
            }

            // Efekty umiejetnosci usuwamy po wygasnieciu.
            bool is_expired_spell = false;
            if (const auto ability_effect = dynamic_cast<Entity::AbilityEffect*>(entity.get()))
                is_expired_spell = ability_effect->isExpired();

            if (entity->isDead() || is_expired_spell) {
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

        for (const auto& candidate : _active_entities)
        {
            if (!candidate || candidate == seeker)
                continue;

            if (candidate->isDead() || candidate->isDying() || candidate->isDormant())
                continue;

            const Entity::EntityType candidate_type = candidate->getType();
            bool is_valid_target = false;

            if (seeker_type == Entity::EntityType::Enemy)
                is_valid_target = candidate_type == Entity::EntityType::Player || candidate_type == Entity::EntityType::Ally;
            else if (seeker_type == Entity::EntityType::Ally)
                is_valid_target = candidate_type == Entity::EntityType::Enemy;

            if (!is_valid_target)
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
        for (auto& entity1 : _active_entities)
        {
            if (entity1->getType() != Entity::EntityType::Projectile) continue;
            if (entity1->isDormant()) continue;

            auto ability = std::static_pointer_cast<Entity::AbilityEffect>(entity1);
            if (ability->isExpired()) continue;

            for (auto& entity2 : _active_entities)
            {
                if (entity1 == entity2) continue;
                if (entity2->isDormant()) continue;

                const Entity::EntityType target_type = entity2->getType();

                if (target_type == Entity::EntityType::Projectile ||
                    target_type == Entity::EntityType::Chest ||
                    target_type == Entity::EntityType::Trigger ||
                    target_type == Entity::EntityType::NPCStatic) continue;

                if (ability->checkCollision(entity2)) {
                    ability->onCollision(entity2);
                }
            }
        }
    }

    void EntityManager::processTriggerCollisions() const
    {
        if (!_player || _player->isDead()) return;

        for (auto& entity : _active_entities) 
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
                    }
                }
            }
        }
    }

    void EntityManager::processPhysicalCollisions() const
    {
        for (size_t i = 0; i < _active_entities.size(); ++i)
        {
            auto& e1 = _active_entities[i];
            if (!isCollidablePhysicalEntity(e1)) continue;

            for (size_t j = i + 1; j < _active_entities.size(); ++j)
            {
                auto& e2 = _active_entities[j];
                if (!isCollidablePhysicalEntity(e2)) continue;

                resolveOverlap(e1, e2);
            }
        }
    }

    bool EntityManager::isCollidablePhysicalEntity(const std::shared_ptr<Entity::Entity>& entity) const {
        if (!entity) return false;
        if (entity->isDead()) return false;
        if (entity->isDormant()) return false;

        const Entity::EntityType type = entity->getType();
        return (type == Entity::EntityType::Player || type == Entity::EntityType::Enemy || type == Entity::EntityType::Ally);
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
        const float combined_radius = 0.8f; 
        
        if (distance_sq < combined_radius * combined_radius && distance_sq > 0.0001f) {
            const float distance = std::sqrt(distance_sq);
            const float overlap = combined_radius - distance;
            const float push_x = (dx / distance) * overlap * 0.5f;
            const float push_y = (dy / distance) * overlap * 0.5f;
            
            first_entity->setX(first_entity->getX() - push_x);
            first_entity->setY(first_entity->getY() - push_y);
            second_entity->setX(second_entity->getX() + push_x);
            second_entity->setY(second_entity->getY() + push_y);
        }
    }

} // namespace Nawia::Core
