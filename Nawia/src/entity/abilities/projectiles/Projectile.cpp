#include "Projectile.h"
#include "EnemyInterface.h"

#include <Logger.h>

#include <cmath>

namespace Nawia::Entity {

	Projectile::Projectile(const float x, const float y, const float target_x, const float target_y, 
	                       const std::shared_ptr<Texture2D> &tex, const AbilityStats& stats, Entity* caster)
		: AbilityEffect(x, y, tex, stats), _speed(stats.projectile_speed), _caster(caster)
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
		if (target.get() == _caster )
			return false;

		// check faction
		if (_caster && _caster->getFaction() == target->getFaction())
			return false;
		
		if (target->isDead())
			return false;

		// ensures projectiles ignore collisions with other spell effects to prevent accidental impacts
		if (std::dynamic_pointer_cast<AbilityEffect>(target))
			return false;

		const float dx = getX() - target->getX();
		const float dy = getY() - target->getY();
		const float distSq = dx * dx + dy * dy;

		bool hit = (distSq < 0.5f);
		if (hit) {
             Core::Logger::debugLog("Projectile collision check: HIT target " + target->getName());
		}
		return hit;
	}

	void Projectile::onCollision(const std::shared_ptr<Entity>& target)
	{
		Core::Logger::debugLog("Projectile::onCollision with " + target->getName());
		target->takeDamage(getDamage());
		die();
		Core::Logger::debugLog("Projectile Hit " + target->getName());
	}

} // namespace Nawia::Entity
