#include "Projectile.h"
#include "EnemyInterface.h"
#include "Collider.h"

#include <Logger.h>

#include <cmath>

namespace Nawia::Entity {

	Projectile::Projectile(const std::string& name, const float x, const float y, const float target_x, const float target_y, 
	                       const std::shared_ptr<Texture2D> &tex, const AbilityStats& stats, Entity* caster)
		: AbilityEffect(name, x, y, tex, stats), _speed(stats.projectile_speed), _caster(caster)
	{
		const float dx = target_x - x;
		const float dy = target_y - y;
		const float length = std::sqrt(dx * dx + dy * dy);
		_vel_x = (dx / length) * _speed;
		_vel_y = (dy / length) * _speed;

		// add Collider
		// hitbox_radius from stats
		setCollider(std::make_unique<CircleCollider>(this, stats.hitbox_radius > 0 ? stats.hitbox_radius : 0.5f));
	}

	void Projectile::update(const float dt) 
	{
		AbilityEffect::update(dt);

		_pos.x += _vel_x * dt;
		_pos.y += _vel_y * dt;
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

		// Use Geometry check
		return AbilityEffect::checkCollision(target);
	}

	void Projectile::onCollision(const std::shared_ptr<Entity>& target)
	{
		Core::Logger::debugLog("Projectile::onCollision with " + target->getName());
		target->takeDamage(getDamage());
		die();
		Core::Logger::debugLog("Projectile Hit " + target->getName());
	}

} // namespace Nawia::Entity
