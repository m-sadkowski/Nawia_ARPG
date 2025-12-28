#include "EntityManager.h"
#include "Logger.h"

#include <AbilityEffect.h>
#include <EnemyInterface.h>
#include <Collider.h>
#include <raylib.h>

#include <cmath>

namespace Nawia::Core {

	void EntityManager::addEntity(std::shared_ptr<Entity::Entity> new_entity) 
	{
		_active_entities.push_back(std::move(new_entity));
	}

	std::shared_ptr<Entity::Entity> EntityManager::getEntityAt(const float screen_x, const float screen_y, const Camera camera) const 
	{
		for (const auto& entity : _active_entities)
			if (entity->isMouseOver(screen_x, screen_y, camera.x, camera.y))
				return entity;

		return nullptr;
	}

	void EntityManager::renderEntities(const Camera &camera) const 
	{
		for (const auto& entity : _active_entities)
			entity->render(camera.x, camera.y);
	}

	void EntityManager::handleEntitiesCollisions() const 
	{
		// 1. Ability Collisions (Projectiles vs Targets)
		for (auto &entity1 : _active_entities) 
		{
			const auto ability = dynamic_cast<Entity::AbilityEffect*>(entity1.get());

			if (!ability || ability->isExpired())
				continue;

			for (auto& entity2 : _active_entities) 
			{
				if (entity1 == entity2)
					continue;

				if (ability->checkCollision(entity2))
					ability->onCollision(entity2);
			}
		}

		// 2. Physical Collisions (Entity vs Entity)
		for (size_t i = 0; i < _active_entities.size(); ++i)
		{
			auto& e1 = _active_entities[i];

			// Skip abilities, dead entities, or those without colliders
			if (dynamic_cast<Entity::AbilityEffect*>(e1.get()) || e1->isDead() || !e1->getCollider())
				continue;

			for (size_t j = i + 1; j < _active_entities.size(); ++j)
			{
				auto& e2 = _active_entities[j];

				if (dynamic_cast<Entity::AbilityEffect*>(e2.get()) || e2->isDead() || !e2->getCollider())
					continue;

				// Check basic collision
				if (e1->getCollider()->checkCollision(e2->getCollider()))
				{
					// Resolve collision (Rectangle vs Rectangle)
					if (e1->getCollider()->getType() == Entity::ColliderType::RECTANGLE &&
						e2->getCollider()->getType() == Entity::ColliderType::RECTANGLE)
					{
						const auto* r1_col = static_cast<const Entity::RectangleCollider*>(e1->getCollider());
						const auto* r2_col = static_cast<const Entity::RectangleCollider*>(e2->getCollider());

						const Rectangle r1_rect = r1_col->getRect();
						const Rectangle r2_rect = r2_col->getRect();

						const Rectangle overlap = GetCollisionRec(r1_rect, r2_rect);

						if (overlap.width > 0 && overlap.height > 0)
						{
							if (overlap.width < overlap.height)
							{
								// Resolve horizontal overlap
								const float half_overlap = overlap.width * 0.5f;
								if (r1_rect.x < r2_rect.x) {
									e1->setX(e1->getX() - half_overlap);
									e2->setX(e2->getX() + half_overlap);
								}
								else {
									e1->setX(e1->getX() + half_overlap);
									e2->setX(e2->getX() - half_overlap);
								}
							}
							else
							{
								// Resolve vertical overlap
								const float half_overlap = overlap.height * 0.5f;
								if (r1_rect.y < r2_rect.y) {
									e1->setY(e1->getY() - half_overlap);
									e2->setY(e2->getY() + half_overlap);
								}
								else {
									e1->setY(e1->getY() + half_overlap);
									e2->setY(e2->getY() - half_overlap);
								}
							}
						}
					}
				}
			}
		}
	}

	void EntityManager::updateEntities(const float delta_time) 
	{
		for (auto it = _active_entities.begin(); it != _active_entities.end();) 
		{
			auto& entity = *it;
			entity->update(delta_time);

			const auto spell_effect = dynamic_cast<Entity::AbilityEffect*>(entity.get());
			const bool expired = (spell_effect && spell_effect->isExpired());

			if (entity->isDead() || expired)
				it = _active_entities.erase(it);
			else
				++it;
		}
	}

} // namespace Nawia::Core