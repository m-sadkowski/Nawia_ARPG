#include "Projectile.h"
#include "Collider.h"
#include "EnemyInterface.h"
#include "ProjectileHitEffect.h"
#include "Player.h"

#include <Logger.h>
#include <Constants.h>
#include <MathUtils.h>

#include <cmath>
#include <raymath.h>

namespace Nawia::Entity {

	Projectile::Projectile(const std::string& name, const float x, const float y, const float target_x, const float target_y,
	                       const std::string& model_path, const float model_scale,
	                       const AbilityStats& stats, Entity* caster,
	                       const std::shared_ptr<Texture2D>& hit_tex,
	                       const float facing_offset)
		: AbilityEffect(name, x, y, nullptr, stats), _speed(stats.projectile_speed), _hit_texture(hit_tex), _caster(caster)
	{
		const float dx = target_x - x;
		const float dy = target_y - y;
		const float length = std::sqrt(dx * dx + dy * dy);
		_vel_x = (dx / length) * _speed;
		_vel_y = (dy / length) * _speed;

		// calculate visual rotation in world space
		const float angle = std::atan2(dy, dx) * 180.0f / PI;
		setRotation(-angle);
		setModelFacingOffset(facing_offset);

		// Load the 3D model for this projectile
		loadModel(model_path);
		setScale(model_scale);

		// Set fly height to roughly torso level
		_fly_height = 1.0f;
	}

	void Projectile::update(const float dt) 
	{
		AbilityEffect::update(dt);

		_pos.x += _vel_x * dt;
		_pos.y += _vel_y * dt;
	}

	bool Projectile::checkCollision(const std::shared_ptr<Entity>& target) const
	{
		if (target.get() == _caster)
			return false;

		// check faction
		if (_caster && _caster->getFaction() == target->getFaction())
			return false;
		
		if (target->isDead())
			return false;

		// ensures projectiles ignore collisions with other spell effects
		if (std::dynamic_pointer_cast<AbilityEffect>(target))
			return false;

		// === 3D Bounding Box Collision ===
		// For projectiles, a bounding box check against the target's bounding box
		// provides reliable collision volume, especially for differently sized models.
		const Vector3 proj_pos = getWorldPos3D();
		const float hit_radius = _stats.hitbox_radius > 0.0f ? _stats.hitbox_radius : 1.5f;

		// Create a bounding box representing the projectile's hit volume
		BoundingBox proj_box = {
			{ proj_pos.x - hit_radius, proj_pos.y - hit_radius, proj_pos.z - hit_radius },
			{ proj_pos.x + hit_radius, proj_pos.y + hit_radius, proj_pos.z + hit_radius }
		};

		BoundingBox target_box = target->getBoundingBox();

		if (CheckCollisionBoxes(proj_box, target_box))
		{
			return true;
		}

		return false;
	}

	void Projectile::onCollision(const std::shared_ptr<Entity>& target)
	{
		Core::Logger::debugLog("Projectile::onCollision with " + target->getName());

		int final_damage = getDamage();

		if (_caster) {
			if (const auto player = dynamic_cast<Player*>(_caster)) {
				final_damage += player->getStats().power;
			}
		}
		if (_caster)
		{
			if (auto playerTarget = std::dynamic_pointer_cast<Player>(target)) {
				final_damage -= playerTarget->getStats().tenacity;
			}
		}
		
		target->takeDamage(final_damage);

		// spawn explosion effect
		if (_caster && _hit_texture) {
			_caster->addPendingSpawn(std::make_unique<ProjectileHitEffect>(_pos.x, _pos.y, _hit_texture));
		}

		die();
		Core::Logger::debugLog("Projectile Hit " + target->getName());
	}

} // namespace Nawia::Entity
