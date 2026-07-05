#include "Entity.h"

#include <Ability.h>
#include <Collider.h>
#include <EntityAbilityConfig.h>
#include <EntityAbilityController.h>
#include <EntityAudioController.h>
#include <EntityMovementSupport.h>
#include <EntityPendingSpawnQueue.h>
#include <EntityTargetingSupport.h>

#include <cmath>
#include <utility>

namespace Nawia::Entity {

	AbilityStats Entity::getAbilityStatsFromJson(const std::string& name)
	{
		return getAbilityStatsFromConfig(name);
	}

	void Entity::addAbility(const std::shared_ptr<Ability>& ability)
	{
		_ability_controller->addAbility(*this, ability);
	}

	void Entity::setAbility(const int index, const std::shared_ptr<Ability>& ability)
	{
		_ability_controller->setAbility(*this, index, ability);
	}

	std::shared_ptr<Ability> Entity::getAbility(const int index)
	{
		return _ability_controller->getAbility(index);
	}

	const std::vector<std::shared_ptr<Ability>>& Entity::getAbilities() const
	{
		return _ability_controller->getAbilities();
	}

	void Entity::updateAbilities(const float dt) const
	{
		_ability_controller->updateAbilities(dt);
	}

	void Entity::addPendingSpawn(std::shared_ptr<Entity> entity)
	{
		_pending_spawn_queue->add(std::move(entity));
	}

	const std::vector<std::shared_ptr<Entity>>& Entity::getPendingSpawns() const
	{
		return _pending_spawn_queue->pending();
	}

	void Entity::clearPendingSpawns()
	{
		_pending_spawn_queue->clear();
	}

	void Entity::setCollider(std::unique_ptr<Collider> collider)
	{
		_collider = std::move(collider);
	}

	void Entity::rotateTowards(const float world_x, const float world_y)
	{
		const float dx = world_x - getX();
		const float dy = world_y - getY();

		if (dx * dx + dy * dy > 0.001f)
		{
			const float angle = std::atan2(dy, dx) * 180.0f / PI;
			setRotation(-angle);
		}
	}

	void Entity::rotateTowardsCenter(const float world_x, const float world_y)
	{
		Vector2 center = getCenter();
		const float dx = world_x - center.x;
		const float dy = world_y - center.y;

		if (dx * dx + dy * dy > 0.001f)
		{
			const float angle = std::atan2(dy, dx) * 180.0f / PI;
			setRotation(-angle);
		}
	}

	void Entity::moveTo(const float x, const float y)
	{
		if (isMovementRooted()) {
			_velocity = {0.0f, 0.0f};
			_is_moving = false;
			return;
		}

		_target_x = x;
		_target_y = y;
		_is_moving = hasMovementTarget(_pos, {_target_x, _target_y});
	}

	void Entity::stopMovement()
	{
		_velocity = {0.0f, 0.0f};
		_is_moving = false;
	}

	void Entity::setMovementTarget(const float x, const float y)
	{
		_target_x = x;
		_target_y = y;
	}

	void Entity::tickPathRecalcTimer(const float dt)
	{
		_path_recalc_timer -= dt;
	}

	bool Entity::isPathRecalcDue() const
	{
		return _path_recalc_timer <= 0.0f;
	}

	void Entity::resetPathRecalcTimer(const float interval)
	{
		_path_recalc_timer = interval;
	}

	void Entity::clearPathRecalcTimer()
	{
		_path_recalc_timer = 0.0f;
	}

	void Entity::updateMovement(const float dt)
	{
		if (isMovementRooted()) {
			_velocity = {0.0f, 0.0f};
			_is_moving = false;
			return;
		}

		if (!_is_moving) return;

		const float move_dist = _movement_speed * _speed_multiplier * dt;
		const MovementAdvanceResult movement = advanceMovementTowards(_pos, {_target_x, _target_y}, move_dist);
		if (movement.should_face_target)
			rotateTowards(_target_x, _target_y);

		_pos = movement.position;
		_is_moving = movement.moving;
	}

	float Entity::getDistanceToTarget() const
	{
		return TargetingSupport::distanceToTarget(*this, _target);
	}

	Vector2 Entity::getTargetPosition() const
	{
		return TargetingSupport::targetPositionOrSelf(*this, _target);
	}

	bool Entity::hasValidTarget() const
	{
		return TargetingSupport::hasLiveTarget(_target);
	}

	void Entity::chaseTarget(const float dt, const float path_recalc_interval)
	{
		if (!hasValidTarget()) return;

		tickPathRecalcTimer(dt);

		if (isPathRecalcDue() || !isMoving())
		{
			const Vector2 target_pos = getTargetPosition();
			moveTo(target_pos.x, target_pos.y);
			resetPathRecalcTimer(path_recalc_interval);
		}

		updateMovement(dt);
	}

	void Entity::playSoundEffect(const std::string& id, const float volume, const bool restart_if_playing, const float pitch) const
	{
		_audio_controller->playSoundEffect(*this, _audio_manager, id, volume, restart_if_playing, pitch);
	}

	void Entity::stopSoundEffect(const std::string& id) const
	{
		_audio_controller->stopSoundEffect(_audio_manager, id);
	}

	void Entity::updateMovementSound(const std::string& path, const bool should_play, const float volume, const float pitch)
	{
		_audio_controller->updateMovementSound(*this, _audio_manager, path, should_play, volume, pitch);
	}

} // namespace Nawia::Entity
