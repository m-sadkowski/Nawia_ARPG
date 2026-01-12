#include "EntityManager.h"
#include "Logger.h"
#include <AbilityEffect.h>
#include <EnemyInterface.h>
#include <Collider.h>
#include <InteractiveTrigger.h>
#include <raylib.h>
#include <cmath>
#include <algorithm> 

namespace Nawia::Core {

    void EntityManager::addEntity(std::shared_ptr<Entity::Entity> new_entity)
    {
        _active_entities.push_back(std::move(new_entity));
    }

    std::shared_ptr<Entity::Entity> EntityManager::getEntityAt(const float screen_x, const float screen_y, const Camera camera) const
    {
        // Iterate backwards to click the "top-most" entity first (rendering order usually)
        for (auto it = _active_entities.rbegin(); it != _active_entities.rend(); ++it) {
            if ((*it)->isMouseOver(screen_x, screen_y, camera.x, camera.y))
                return *it;
        }
        return nullptr;
    }

    void EntityManager::renderEntities(const Camera& camera) const
    {
        for (const auto& entity : _active_entities)
            entity->render(camera.x, camera.y);
    }

    void EntityManager::updateEntities(const float delta_time)
    {
        for (auto it = _active_entities.begin(); it != _active_entities.end();)
        {
            auto& entity = *it;
            entity->update(delta_time);

            // Check if it's an expired spell
            bool is_expired_spell = false;
            if (auto spell = dynamic_cast<Entity::AbilityEffect*>(entity.get())) {
                is_expired_spell = spell->isExpired();
            }

            if (entity->isDead() || is_expired_spell) {
                it = _active_entities.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    // --- Collision System Refactor ---

    void EntityManager::handleEntitiesCollisions() 
    {
        processAbilityCollisions();
        processTriggerCollisions();
        processPhysicalCollisions();
    }

    void EntityManager::processAbilityCollisions() 
    {
        for (auto& entity1 : _active_entities)
        {
            auto ability = dynamic_cast<Entity::AbilityEffect*>(entity1.get());
            if (!ability || ability->isExpired()) continue;

            for (auto& entity2 : _active_entities)
            {
                if (entity1 == entity2) continue;

                // Abilities ignore interactive objects (like signs/chests)
                if (dynamic_cast<Entity::Interactable*>(entity2.get())) continue;

                if (ability->checkCollision(entity2)) {
                    ability->onCollision(entity2);
                }
            }
        }
    }

    void EntityManager::processTriggerCollisions() 
    {
        if (!_player || _player->isDead()) return;

        for (auto& entity : _active_entities) {
            if (entity == _player) continue;

            if (auto trigger = dynamic_cast<Entity::InteractiveTrigger*>(entity.get())) {
                // Ensure trigger has a collider before checking
                if (trigger->getCollider() &&
                    trigger->getCollider()->checkCollision(_player->getCollider()))
                {
                    trigger->onTriggerEnter(*_player);
                    trigger->die();
                }
            }
        }
    }

    void EntityManager::processPhysicalCollisions()
    {
        for (size_t i = 0; i < _active_entities.size(); ++i)
        {
            auto& e1 = _active_entities[i];
            if (!isCollidablePhysicalEntity(e1)) continue;

            for (size_t j = i + 1; j < _active_entities.size(); ++j)
            {
                auto& e2 = _active_entities[j];
                if (!isCollidablePhysicalEntity(e2)) continue;

                if (e1->getCollider()->checkCollision(e2->getCollider())) {
                    resolveOverlap(e1, e2);
                }
            }
        }
    }

    // Helper to filter entities that participate in physics
    bool EntityManager::isCollidablePhysicalEntity(const std::shared_ptr<Entity::Entity>& e) const
    {
        if (e->isDead() || !e->getCollider()) return false;

        // Ignore specific types for physics resolution
        if (dynamic_cast<Entity::AbilityEffect*>(e.get())) return false;
        if (dynamic_cast<Entity::Interactable*>(e.get())) return false;

        return true;
    }

    void EntityManager::resolveOverlap(std::shared_ptr<Entity::Entity>& e1, std::shared_ptr<Entity::Entity>& e2) const
    {
        // Only resolve Rectangle vs Rectangle for now
        if (e1->getCollider()->getType() != Entity::ColliderType::RECTANGLE ||
            e2->getCollider()->getType() != Entity::ColliderType::RECTANGLE) {
            return;
        }

        const auto* r1_col = static_cast<const Entity::RectangleCollider*>(e1->getCollider());
        const auto* r2_col = static_cast<const Entity::RectangleCollider*>(e2->getCollider());

        const Rectangle r1_rect = r1_col->getRect();
        const Rectangle r2_rect = r2_col->getRect();

        Rectangle overlap = GetCollisionRec(r1_rect, r2_rect);

        if (overlap.width <= 0 || overlap.height <= 0) return;

        // Determine smallest axis of penetration
        if (overlap.width < overlap.height)
        {
            const float separation = overlap.width * 0.5f;
            // Push apart horizontally
            if (r1_rect.x < r2_rect.x) {
                e1->setX(e1->getX() - separation);
                e2->setX(e2->getX() + separation);
            }
            else {
                e1->setX(e1->getX() + separation);
                e2->setX(e2->getX() - separation);
            }
        }
        else
        {
            const float separation = overlap.height * 0.5f;
            // Push apart vertically
            if (r1_rect.y < r2_rect.y) {
                e1->setY(e1->getY() - separation);
                e2->setY(e2->getY() + separation);
            }
            else {
                e1->setY(e1->getY() + separation);
                e2->setY(e2->getY() - separation);
            }
        }
    }

} // namespace Nawia::Core