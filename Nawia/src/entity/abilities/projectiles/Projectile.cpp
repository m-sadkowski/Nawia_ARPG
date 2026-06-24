#include "Projectile.h"

#include <Constants.h>
#include <EnemyInterface.h>
#include <Logger.h>
#include <MathUtils.h>
#include <Player.h>
#include <ProjectileHitEffect.h>
#include <SoundIds.h>


#include <raymath.h>

#include <algorithm>
#include <cmath>

namespace Nawia::Entity {

	namespace {
		constexpr float MIN_DIRECTION_LENGTH = 0.001f;
		constexpr float DEFAULT_HIT_RADIUS = 1.5f;
	}

	Projectile::Projectile(const std::string& name,
						   const float x,
						   const float y,
						   const float target_x,
						   const float target_y,
						   const std::string& model_path,
						   const float model_scale,
						   const AbilityStats& stats,
						   Entity* caster,
						   const float target_height,
						   const std::shared_ptr<Texture2D>& hit_tex,
						   const float facing_offset,
						   const Model* shared_model)
		: AbilityEffect(name, x, y, nullptr, stats),
		  _speed(stats.projectile_speed),
		  _hit_texture(hit_tex),
		  _caster(caster) {
		if (_caster) {
			const BoundingBox caster_box = _caster->getBoundingBox();
			_start_height = (caster_box.min.y + caster_box.max.y) * 0.5f;
		}
		_target_height = target_height;
		_flight_height = _start_height;

		configureMovement(target_x, target_y, facing_offset);
		setAltitude(_caster ? _caster->getAltitude() : 0.0f);

		if (shared_model) {
			useSharedModel(*shared_model);
			getModel().transform = MatrixIdentity();
		} else {
			loadModel(model_path);
		}
		setScale(model_scale);
	}

	void Projectile::configureMovement(const float target_x, const float target_y, const float facing_offset) {
		const float dx = target_x - getX();
		const float dy = target_y - getY();
		const float length = std::sqrt(dx * dx + dy * dy);

		_start_x = getX();
		_start_y = getY();
		_travel_distance = length;

		if (length <= MIN_DIRECTION_LENGTH || _speed <= 0.0f) {
			_vel_x = 0.0f;
			_vel_y = 0.0f;
			return;
		}

		_vel_x = (dx / length) * _speed;
		_vel_y = (dy / length) * _speed;

		const float angle = std::atan2(dy, dx) * 180.0f / PI;
		setRotation(-angle);
		setModelFacingOffset(facing_offset);
	}

	void Projectile::update(const float dt) {
		AbilityEffect::update(dt);

		_pos.x += _vel_x * dt;
		_pos.y += _vel_y * dt;

		if (_travel_distance > MIN_DIRECTION_LENGTH) {
			const float traveled_x = _pos.x - _start_x;
			const float traveled_y = _pos.y - _start_y;
			const float traveled_distance = std::sqrt(traveled_x * traveled_x + traveled_y * traveled_y);
			const float progress = std::clamp(traveled_distance / _travel_distance, 0.0f, 1.0f);
			_flight_height = _start_height + (_target_height - _start_height) * progress;
		}
	}

	bool Projectile::checkCollision(const std::shared_ptr<Entity>& target) const {
		if (!canHitTarget(target) || isDead())
			return false;

		if (target.get() == _caster)
			return false;

		if (_caster && _caster->getFaction() != Faction::None && _caster->getFaction() == target->getFaction())
			return false;

		if (_caster && _caster->getFaction() == Faction::Player && target->getFaction() == Faction::Ally)
			return false;

		if (std::dynamic_pointer_cast<AbilityEffect>(target))
			return false;

		const Vector3 projectile_position = getWorldPos3D();
		const float hit_radius = _stats.hitbox_radius > 0.0f ? _stats.hitbox_radius : DEFAULT_HIT_RADIUS;

		const BoundingBox projectile_box = {
			{projectile_position.x - hit_radius, projectile_position.y - hit_radius, projectile_position.z - hit_radius},
			{projectile_position.x + hit_radius, projectile_position.y + hit_radius, projectile_position.z + hit_radius}
		};

		return CheckCollisionBoxes(projectile_box, target->getBoundingBox());
	}

	void Projectile::onCollision(const std::shared_ptr<Entity>& target) {
		if (!target || isDead())
			return;

		int final_damage = getDamage();

		if (const auto player = dynamic_cast<Player*>(_caster))
			final_damage += player->getStats().power;

		target->rememberDamageSource(_caster);
		target->takeDamage(final_damage);
		addHit(target);

		if (_name == "Fireball")
			playSoundEffect(Audio::SoundId::FireballHit, 0.9f);

		if (_hit_texture)
			addPendingSpawn(std::make_shared<ProjectileHitEffect>(_pos.x, _pos.y, _hit_texture));

		die();
		Core::Logger::debugLog("Projectile trafil " + target->getName() + " za " + std::to_string(final_damage) + " obrazen.");
	}

} // namespace Nawia::Entity
