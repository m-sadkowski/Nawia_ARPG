#include "EntityManager.h"
#include "Logger.h"

#include <AbilityEffect.h>
#include <EnemyInterface.h>

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
		for (auto &entity1 : _active_entities) 
		{
			const auto ability = dynamic_cast<Entity::AbilityEffect*>(entity1.get());

			if (!ability || ability->isExpired())
				return;

			for (auto& entity2 : _active_entities) 
			{
				if (entity1 == entity2)
					continue;

				if (ability->checkCollision(entity2))
					ability->onCollision(entity2);
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