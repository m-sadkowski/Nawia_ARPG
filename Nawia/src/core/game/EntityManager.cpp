#include "EntityManager.h"
#include <AbilityEffect.h>
#include <InteractiveObject.h>
#include <Trigger.h>
#include <Player.h>
#include <Enemy.h>
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
	  for (const auto &entity : _active_entities) 
	    if (entity->isMouseOver(screen_x, screen_y, camera.x, camera.y))
	      return entity;

	  return nullptr;
	}

	void EntityManager::renderEntities(SDL_Renderer *renderer, const Camera& camera) const 
	{
	  for (const auto &entity : _active_entities) 
	    entity->render(renderer, camera.x, camera.y);
	}

	void EntityManager::handleEntitiesCollisions() const
	{

		Entity::Player* player = nullptr;
		for (auto& entity : _active_entities) {
			if (auto p = dynamic_cast<Entity::Player*>(entity.get())) {
				player = p;
				break;
			}
		}

		for (auto& entity1 : _active_entities)
		{
			


			if (const auto ability = dynamic_cast<Entity::AbilityEffect*>(entity1.get()))
			{
				if (ability->isExpired())
					continue;

				for (auto& entity2 : _active_entities)
				{
					if (entity1 == entity2)
						continue;

					if (ability->checkCollision(entity2))
						ability->onCollision(entity2);
				}
			}
			else if (const auto trigger = dynamic_cast<Entity::Trigger*>(entity1.get())){

				if (!trigger->isInteractive()) continue;

				// Trigger sprawdzamy TYLKO wzgl�dem gracza
				if (player && trigger->checkCollision(player))
				{
					trigger->interaction(); // Aktywuj np. checkpoint lub pu�apk�
				}

			}

		}
	}

	void EntityManager::updateEntities(const float delta_time) 
	{
	  for (auto it = _active_entities.begin(); it != _active_entities.end();) 
	  {
	    auto &entity = *it;
	    entity->update(delta_time);

	    const auto spell_effect = dynamic_cast<Entity::AbilityEffect *>(entity.get());
	    const bool expired = (spell_effect && spell_effect->isExpired());

	    if (entity->isDead() || expired)
	      it = _active_entities.erase(it);
	    else
	      ++it;
	  }
	}

} // namespace Nawia::Core