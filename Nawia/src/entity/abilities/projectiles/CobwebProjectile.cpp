#include "CobwebProjectile.h"

#include <Logger.h>
#include <Player.h>
#include <SoundIds.h>

#include <raymath.h>

#include <algorithm>
#include <cmath>

namespace Nawia::Entity {

	namespace {
		constexpr const char* COBWEB_MODEL = "assets/models/cobweb.glb";
		constexpr float HIT_RADIUS = 0.7f;
		constexpr float STUCK_DURATION = 2.0f;
		constexpr int COBWEB_DAMAGE = 10;
		constexpr float MIN_DIRECTION_LENGTH = 0.001f;

		AbilityStats makeCobwebStats() {
			AbilityStats stats;
			stats.damage = COBWEB_DAMAGE;
			stats.duration = 3.5f;
			stats.projectile_speed = 9.0f;
			stats.hitbox_radius = HIT_RADIUS;
			return stats;
		}
	}

	CobwebProjectile::CobwebProjectile(
		const float x,
		const float y,
		const float target_x,
		const float target_y,
		const float start_height,
		const float target_height,
		const Model* shared_model,
		Entity* caster)
		: AbilityEffect("Cobweb", x, y, nullptr, makeCobwebStats()),
		  _speed(_stats.projectile_speed),
		  _start_height(start_height),
		  _target_height(target_height),
		  _flight_height(start_height),
		  _caster(caster)
	{
		configureMovement(target_x, target_y);
		setAltitude(caster ? caster->getAltitude() : 0.0f);

		if (shared_model) {
			useSharedModel(*shared_model);
			getModel().transform = MatrixIdentity();
		} else {
			loadModel(COBWEB_MODEL);
		}
		setScale(0.85f);
		setModelFacingOffset(0.0f);
	}

	void CobwebProjectile::configureMovement(const float target_x, const float target_y) {
		const float dx = target_x - getX();
		const float dy = target_y - getY();
		const float length = std::sqrt(dx * dx + dy * dy);

		_start_x = getX();
		_start_y = getY();
		_travel_distance = length;

		if (length <= MIN_DIRECTION_LENGTH || _speed <= 0.0f)
			return;

		_vel_x = dx / length * _speed;
		_vel_y = dy / length * _speed;

		const float angle = std::atan2(dy, dx) * 180.0f / PI;
		setRotation(-angle);
	}

	void CobwebProjectile::update(const float dt) {
		AbilityEffect::update(dt);

		if (auto attached_target = _attached_target.lock()) {
			_attached_timer -= dt;
			updateAttachedPosition();
			if (_attached_timer <= 0.0f || attached_target->isDead() || attached_target->isDying())
				die();
			return;
		}

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

	void CobwebProjectile::render(const Camera3D& camera) {
		if (auto attached_target = _attached_target.lock())
			updateAttachedPosition();

		Entity::render(camera);
	}

	bool CobwebProjectile::checkCollision(const std::shared_ptr<Entity>& target) const {
		if (!canHitTarget(target) || isDead() || !_attached_target.expired())
			return false;

		if (target.get() == _caster)
			return false;

		const EntityType target_type = target->getType();
		if (target_type != EntityType::Player && target_type != EntityType::Ally)
			return false;

		const Vector3 projectile_position = getWorldPos3D();
		const float hit_radius = _stats.hitbox_radius > 0.0f ? _stats.hitbox_radius : HIT_RADIUS;
		const BoundingBox projectile_box = {
			{projectile_position.x - hit_radius, projectile_position.y - hit_radius, projectile_position.z - hit_radius},
			{projectile_position.x + hit_radius, projectile_position.y + hit_radius, projectile_position.z + hit_radius}
		};

		return CheckCollisionBoxes(projectile_box, target->getBoundingBox());
	}

	void CobwebProjectile::onCollision(const std::shared_ptr<Entity>& target) {
		if (!target || isDead() || !_attached_target.expired())
			return;

		target->rememberDamageSource(_caster, getName());
		target->takeDamage(getDamage());
		target->applyRoot(STUCK_DURATION);
		if (auto* player = dynamic_cast<Player*>(target.get()))
			player->applyControlLock(STUCK_DURATION);
		addHit(target);

		if (_caster)
			_caster->setTarget(target);

		_attached_target = target;
		_attached_timer = STUCK_DURATION;
		_timer = 0.0f;
		updateAttachedPosition();
		playSoundEffect(Audio::SoundId::FireballHit, 0.55f, true, 1.35f);
		Core::Logger::debugLog("Pajeczyna trafila " + target->getName() + ".");
	}

	void CobwebProjectile::updateAttachedPosition() {
		const auto target = _attached_target.lock();
		if (!target)
			return;

		const BoundingBox target_box = target->getBoundingBox();
		_pos.x = (target_box.min.x + target_box.max.x) * 0.5f;
		_pos.y = (target_box.min.z + target_box.max.z) * 0.5f;
		_flight_height = (target_box.min.y + target_box.max.y) * 0.5f;
	}

} // namespace Nawia::Entity
