#include "EntityManager.h"
#include "Logger.h"
#include "Engine.h"

#include <AbilityEffect.h>
#include <EnemyInterface.h>
#include <Collider.h>
#include <InteractiveTrigger.h>

#include <raylib.h>
#include <cmath>
#include <algorithm>
#include <limits>

namespace Nawia::Core {

    void EntityManager::addEntity(std::shared_ptr<Entity::Entity> new_entity)
    {
        _active_entities.push_back(std::move(new_entity));
    }

	void EntityManager::clearNonPlayerEntities() {
		std::vector<std::shared_ptr<Entity::Entity>> new_entities;
		if (_player) 
        {
			new_entities.push_back(_player);
		} 
    	else 
    	{
			for (const auto& entity : _active_entities) {
				if (entity->getName() == "Player") {
					new_entities.push_back(entity);
					break;
				}
			}
		}
		_active_entities = std::move(new_entities);
	}

    std::shared_ptr<Entity::Entity> EntityManager::getEntityAt(const float screen_x, const float screen_y, const Camera3D& camera) const
    {
        // Iterate backwards to click the "top-most" entity first
        for (auto it = _active_entities.rbegin(); it != _active_entities.rend(); ++it) {
            if ((*it)->isDormant()) continue;
            if ((*it)->isMouseOver(screen_x, screen_y, camera))
                return *it;
        }
        return nullptr;
    }

    void EntityManager::updateHoverState(const float screen_x, const float screen_y, const Camera3D& camera)
    {
        // 1. Reset hover state for all active entities
        for (const auto& entity : _active_entities)
            entity->setHovered(false);

        // 2. Find the top-most entity under the cursor
        for (auto it = _active_entities.rbegin(); it != _active_entities.rend(); ++it) 
        {
            if ((*it)->isDormant()) continue;
            if ((*it)->isMouseOver(screen_x, screen_y, camera)) {
                (*it)->setHovered(true);
                return;
            }
        }
    }

    void EntityManager::renderEntities(const Camera3D& camera) const
    {
        std::vector<Entity::Entity*> render_list;
        render_list.reserve(_active_entities.size());

        for (const auto& entity : _active_entities)
        {
            if (!entity->isDormant())
                render_list.push_back(entity.get());
        }

		// Y-sorting
        std::ranges::sort(render_list, {}, &Entity::Entity::getY);

        for (auto* entity : render_list) {
            entity->render(camera);
        }
    }

    void EntityManager::updateEntities(const float delta_time)
    {
        refreshCombatTargets();

        for (auto it = _active_entities.begin(); it != _active_entities.end();)
        {
            const auto& entity = *it;
            entity->update(delta_time);

            // Check if it's an expired spell
            bool is_expired_spell = false;
            if (const auto spell = dynamic_cast<Entity::AbilityEffect*>(entity.get()))
                is_expired_spell = spell->isExpired();

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

    void EntityManager::handleEntitiesCollisions() const
    {
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

                Entity::EntityType targetType = entity2->getType();

                if (targetType == Entity::EntityType::Projectile ||
                    targetType == Entity::EntityType::Chest ||
                    targetType == Entity::EntityType::Trigger ||
                    targetType == Entity::EntityType::NPCStatic) continue;

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
            if (entity == _player) continue;

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

    bool EntityManager::isCollidablePhysicalEntity(const std::shared_ptr<Entity::Entity>& e) const
    {
        if (e->isDead()) return false;
        if (e->isDormant()) return false;

        const Entity::EntityType type = e->getType();
        return (type == Entity::EntityType::Player || type == Entity::EntityType::Enemy || type == Entity::EntityType::Ally || type == Entity::EntityType::Wall);
    }

    void EntityManager::resolveOverlap(const std::shared_ptr<Entity::Entity>& e1, const std::shared_ptr<Entity::Entity>& e2) const
    {
        const bool e1_wall = (e1->getType() == Entity::EntityType::Wall);
        const bool e2_wall = (e2->getType() == Entity::EntityType::Wall);

        // Wall vs Wall — no resolution needed
        if (e1_wall && e2_wall) return;

        // Wall vs Character — use AABB collision
        if (e1_wall || e2_wall) {
            const auto& wall = e1_wall ? e1 : e2;
            const auto& character = e1_wall ? e2 : e1;

            auto* rect = dynamic_cast<Entity::RectangleCollider*>(wall->getCollider());
            if (!rect) return;

            const Vector2 wall_center = rect->getPosition();
            const float half_w = rect->getWidth() / 2.0f;
            const float half_h = rect->getHeight() / 2.0f;

            const float char_x = character->getX();
            const float char_y = character->getY();
            constexpr float char_radius = 0.4f;

            // Find closest point on wall AABB to character
            const float closest_x = std::clamp(char_x, wall_center.x - half_w, wall_center.x + half_w);
            const float closest_y = std::clamp(char_y, wall_center.y - half_h, wall_center.y + half_h);

            const float dx = char_x - closest_x;
            const float dy = char_y - closest_y;
            const float dist_sq = dx * dx + dy * dy;

            if (dist_sq < char_radius * char_radius) {
                if (dist_sq > 0.0001f) {
                    const float dist = std::sqrt(dist_sq);
                    const float overlap = char_radius - dist;
                    const float push_x = (dx / dist) * overlap;
                    const float push_y = (dy / dist) * overlap;
                    character->setX(char_x + push_x);
                    character->setY(char_y + push_y);
                } else {
                    // Character is exactly at closest point (inside wall) — push out along shortest axis
                    const float pen_x = half_w - std::abs(char_x - wall_center.x) + char_radius;
                    const float pen_y = half_h - std::abs(char_y - wall_center.y) + char_radius;
                    if (pen_x < pen_y) {
                        character->setX(char_x + (char_x < wall_center.x ? -pen_x : pen_x));
                    } else {
                        character->setY(char_y + (char_y < wall_center.y ? -pen_y : pen_y));
                    }
                }
            }
            return;
        }

        // Character vs Character — radial push
        const float dx = e2->getX() - e1->getX();
        const float dy = e2->getY() - e1->getY();
        const float dist_sq = dx * dx + dy * dy;
        
        constexpr float combined_radius = 0.8f; 
        
        if (dist_sq < combined_radius * combined_radius && dist_sq > 0.0001f) {
            float dist = std::sqrt(dist_sq);
            float overlap = combined_radius - dist;
            float push_x = (dx / dist) * overlap * 0.5f;
            float push_y = (dy / dist) * overlap * 0.5f;
            
            e1->setX(e1->getX() - push_x);
            e1->setY(e1->getY() - push_y);
            e2->setX(e2->getX() + push_x);
            e2->setY(e2->getY() + push_y);
        }
    }

} // namespace Nawia::Core
