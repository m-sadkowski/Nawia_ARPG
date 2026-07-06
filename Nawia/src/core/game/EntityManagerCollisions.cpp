#include "EntityManager.h"

#include <AbilityEffect.h>
#include <BossTelegraphHazard.h>
#include <Collider.h>
#include <Entity.h>
#include <InteractiveTrigger.h>

#include <algorithm>
#include <cmath>

namespace Nawia::Core {

    namespace {

        constexpr float k_physical_collision_radius = 0.8f;

        bool isAbilityTarget(const std::shared_ptr<Entity::Entity>& entity) {
            if (!entity || entity->isDormant())
                return false;

            const Entity::EntityType type = entity->getType();
            return type != Entity::EntityType::Projectile &&
                   type != Entity::EntityType::Hazard &&
                   type != Entity::EntityType::Chest &&
                   type != Entity::EntityType::Trigger &&
                   type != Entity::EntityType::NPCStatic &&
                   type != Entity::EntityType::NPCActor;
        }

    } // namespace

    void EntityManager::handleEntitiesCollisions() const {
        processAbilityCollisions();
        processHazardEffects();
        processTriggerCollisions();
        processPhysicalCollisions();
    }

    void EntityManager::processAbilityCollisions() const {
        std::vector<std::shared_ptr<Entity::Entity>> targets;
        targets.reserve(_active_entities.size());
        for (const auto& entity : _active_entities) {
            if (isAbilityTarget(entity))
                targets.push_back(entity);
        }

        for (const auto& entity1 : _active_entities) {
            if (entity1->getType() != Entity::EntityType::Projectile) continue;
            if (entity1->isDormant()) continue;

            auto ability = std::static_pointer_cast<Entity::AbilityEffect>(entity1);
            if (ability->isExpired()) continue;

            for (const auto& entity2 : targets) {
                if (entity1 == entity2) continue;

                if (ability->checkCollision(entity2))
                    ability->onCollision(entity2);
            }
        }
    }

    void EntityManager::processHazardEffects() const {
        std::vector<std::shared_ptr<Entity::Entity>> targets;
        targets.reserve(_active_entities.size());
        for (const auto& entity : _active_entities) {
            if (!entity || entity->isDormant() || entity->isDead() || entity->isDying())
                continue;

            const Entity::EntityType type = entity->getType();
            if (type == Entity::EntityType::Player || type == Entity::EntityType::Ally || type == Entity::EntityType::Enemy)
                targets.push_back(entity);
        }

        for (const auto& entity : _active_entities) {
            auto* hazard = dynamic_cast<Entity::BossTelegraphHazard*>(entity.get());
            if (!hazard || hazard->isDormant() || hazard->isDead() || !hazard->isActiveHazard())
                continue;

            for (const auto& target : targets) {
                if (target != entity)
                    hazard->applyToTarget(target);
            }
        }
    }

    void EntityManager::processTriggerCollisions() const {
        if (!_player || _player->isDead())
            return;

        const auto entities_snapshot = _active_entities;
        for (const auto& entity : entities_snapshot) {
            if (!entity || entity == _player) continue;
            if (entity->isDormant()) continue;

            if (const auto trigger = dynamic_cast<Entity::InteractiveTrigger*>(entity.get())) {
                if (trigger->getCollider()) {
                    bool collision = false;
                    if (_player->getCollider())
                        collision = trigger->getCollider()->checkCollision(_player->getCollider());
                    else
                        collision = trigger->getCollider()->checkCollision(_player->getBoundingBox());

                    if (collision) {
                        trigger->onTriggerEnter(*_player);
                        return;
                    }
                }
            }
        }
    }

    void EntityManager::processPhysicalCollisions() const {
        std::vector<std::shared_ptr<Entity::Entity>> collidable_entities;
        collidable_entities.reserve(_active_entities.size());
        for (const auto& entity : _active_entities) {
            if (isCollidablePhysicalEntity(entity))
                collidable_entities.push_back(entity);
        }

        std::ranges::sort(collidable_entities, [](const auto& left, const auto& right) {
            return left->getX() < right->getX();
        });

        for (size_t i = 0; i < collidable_entities.size(); ++i) {
            const auto& e1 = collidable_entities[i];

            for (size_t j = i + 1; j < collidable_entities.size(); ++j) {
                const auto& e2 = collidable_entities[j];
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
        const float dx = second_entity->getX() - first_entity->getX();
        const float dy = second_entity->getY() - first_entity->getY();
        const float distance_sq = dx * dx + dy * dy;
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
