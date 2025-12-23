#include "Projectile.h"
#include "EnemyInterface.h"

#include <Logger.h>

#include <cmath>

namespace Nawia::Entity {

	Projectile::Projectile(const float x, const float y, const float target_x, const float target_y, 
	                       const std::shared_ptr<Texture2D> &tex, const AbilityStats& stats)
		: AbilityEffect(x, y, tex, stats), _speed(stats.projectile_speed)
	{
		const float dx = target_x - x;
		const float dy = target_y - y;
		const float length = std::sqrt(dx * dx + dy * dy);
		_vel_x = (dx / length) * _speed;
		_vel_y = (dy / length) * _speed;
	}

	void Projectile::update(const float dt) 
	{
		AbilityEffect::update(dt);

		_pos->setX(_pos->getX() + _vel_x * dt);
		_pos->setY(_pos->getY() + _vel_y * dt);
	}

	bool Projectile::checkCollision(const std::shared_ptr<Entity>& target) const
	{
		const auto enemy = std::dynamic_pointer_cast<EnemyInterface>(target);

		if (!enemy || enemy->isDead())
			return false;

		const float dx = getX() - enemy->getX();
		const float dy = getY() - enemy->getY();
		return (dx * dx + dy * dy < 0.25f);
	}

	void Projectile::onCollision(const std::shared_ptr<Entity>& target)
	{
		if (const auto enemy = std::dynamic_pointer_cast<EnemyInterface>(target))
		{
			enemy->takeDamage(getDamage());
			die();
			Core::Logger::debugLog("Hit enemy!");
		}
	}

} // namespace Nawia::Entity
