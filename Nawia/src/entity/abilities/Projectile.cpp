#include "Projectile.h"
#include "Enemy.h"
#include "Logger.h"
#include <cmath>

namespace Nawia::Entity {

	Projectile::Projectile(const float x, const float y, const float target_x, const float target_y, const float speed, const std::shared_ptr<SDL_Texture> &tex, const int damage, const float duration)
	    : AbilityEffect(x, y, tex, duration, damage), _speed(speed) 
	{
	  // calculate direction vector
	  const float dx = target_x - x;
	  const float dy = target_y - y;
	  const float length = std::sqrt(dx * dx + dy * dy);
	  _vel_x = (dx / length) * speed;
	  _vel_y = (dy / length) * speed;
	}

	void Projectile::update(const float dt) 
	{
	  AbilityEffect::update(dt);

	  _pos->setX(_pos->getX() + _vel_x * dt);
	  _pos->setY(_pos->getY() + _vel_y * dt);
	}

	bool Projectile::checkCollision(const std::shared_ptr<Entity>& target) const
	{
		if (const auto enemy = std::dynamic_pointer_cast<Enemy>(target))
	  {
	    if (enemy->isDead())
	      return false;

	    const float dx = getX() - enemy->getX();
	    const float dy = getY() - enemy->getY();
	    return (dx * dx + dy * dy < 0.25f);
	  }
	  return false;
	}

	void Projectile::onCollision(const std::shared_ptr<Entity>& target)
	{
		if (const auto enemy = std::dynamic_pointer_cast<Enemy>(target))
	  {
	    enemy->takeDamage(getDamage());
	    takeDamage(9999);
	    Core::Logger::debugLog("Hit Enemy!");
	  }
	}

} // namespace Nawia::Entity
