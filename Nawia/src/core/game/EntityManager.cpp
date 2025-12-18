#include "EntityManager.h"
#include "Enemy.h"
#include "Logger.h"
#include "Projectile.h"

namespace Nawia::Core
{

	void EntityManager::addEntity(std::shared_ptr<Entity::Entity> new_entity)
	{
		_active_entities.push_back(std::move(new_entity));
	}
	
	std::shared_ptr<Entity::Entity> EntityManager::getEntityAt(const float screen_x, const float screen_y, const Camera camera) const {
		for (const auto& entity : _active_entities) {
			if (entity->isMouseOver(screen_x, screen_y, camera.x, camera.y)) {
				return entity;
			}
		}
		return nullptr;
	}

	void EntityManager::renderEntities(SDL_Renderer* renderer, const Camera camera) const 
	{
		for (const auto& entity : _active_entities) {
			entity->render(renderer, camera.x, camera.y);
		}
	}

	void EntityManager::handleEntitiesCollisions() const
	{
		for (auto& entity1 : _active_entities) {
			if (const auto projectile = dynamic_cast<Entity::Projectile*>(entity1.get())) {
				if (projectile->isExpired())
					continue;

				for (auto& entity2 : _active_entities) {
					if (entity1 == entity2)
						continue; // skip self

					if (const auto enemy = dynamic_cast<Entity::Enemy*>(entity2.get())) {
						if (enemy->isDead())
							continue;

						const float dx = projectile->getX() - enemy->getX();
						const float dy = projectile->getY() - enemy->getY();

						if (dx * dx + dy * dy < 0.25f) {
							enemy->takeDamage(projectile->getDamage());
							projectile->takeDamage(9999);
							Logger::debugLog("Hit Enemy!");
						}
					}
				}
			}
		}
	}

	void EntityManager::updateEntities(const float delta_time) 
	{
		for (auto it = _active_entities.begin(); it != _active_entities.end();) {
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