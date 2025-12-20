#include "EntityManager.h"
#include "Enemy.h"
#include "Logger.h"
#include "Projectile.h"
#include "SwordSlashEffect.h"
#include <cmath>

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

	// IT NEEDS TO BE SIMPLIFIED AND BE MORE FLEXIBLE, WE NEED SEPERATE FUNCTIONS FOR DIFFERENT SHAPES - CONES, CIRCLES, RECTANGLES
	// BEACUSE THE ONLY DIFFERENCE BETWEEN PROJECTILE AND SWORDSLASH FOR EXAMPLE IS THEIR EFFECT SHAPE AND ANGLE
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
  		else if (const auto sword_slash = dynamic_cast<Entity::SwordSlashEffect* >(entity1.get())) {
	      if (sword_slash->isExpired())
	        continue;

	      for (auto &entity2 : _active_entities) {
	        if (entity1 == entity2)
	          continue;

	        if (const auto enemy = dynamic_cast<Entity::Enemy *>(entity2.get())) {
	          if (enemy->isDead())
	            continue;

	          if (sword_slash->hasHit(enemy))
	            continue;

	          const float dx = enemy->getX() - sword_slash->getX();
	          const float dy = enemy->getY() - sword_slash->getY();
	          const float distance_squared = dx * dx + dy * dy;

	          // Range check (e.g., 2.5 tiles)
	          if (distance_squared < 2.5f * 2.5f) {
	            // Angle check
	            // Calculate angle to enemy
	            float angle_to_enemy = std::atan2(dy, dx) * 180.0f / 3.14159f;
	            float slash_angle = sword_slash->getAngle();

	            // Normalize angles to -180 to 180
	            float angle_diff = angle_to_enemy - slash_angle;
	            while (angle_diff > 180.0f)
	              angle_diff -= 360.0f;
	            while (angle_diff < -180.0f)
	              angle_diff += 360.0f;

	            if (std::abs(angle_diff) < 45.0f) { // 90 degree cone
	              enemy->takeDamage(sword_slash->getDamage());
	              sword_slash->addHit(enemy);
	              Logger::debugLog("Sword Slash Hit Enemy!");
	            }
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