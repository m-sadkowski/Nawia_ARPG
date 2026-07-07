#include "Entity.h"

#include <Ability.h>
#include <Collider.h>
#include <EntityAbilityConfig.h>
#include <EntityAbilityController.h>
#include <EntityAudioController.h>
#include <EntityMovementState.h>
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

	void Entity::setVelocity(const float x, const float y)
	{
		_movement_state->velocity = {x, y};
	}

	Vector2 Entity::getVelocity() const
	{
		return _movement_state->velocity;
	}

	void Entity::setScale(const float scale)
	{
		_movement_state->scale = scale;
	}

	float Entity::getScale() const
	{
		return _movement_state->scale;
	}

	void Entity::setRotation(const float angle)
	{
		_movement_state->rotation = angle;
	}

	float Entity::getRotation() const
	{
		return _movement_state->rotation;
	}

	void Entity::setMovementSpeed(const float speed)
	{
		_movement_state->movement_speed = speed;
	}

	float Entity::getMovementSpeed() const
	{
		return _movement_state->movement_speed;
	}

	void Entity::setSpeedMultiplier(const float multiplier)
	{
		_movement_state->speed_multiplier = multiplier;
	}

	float Entity::getSpeedMultiplier() const
	{
		return _movement_state->speed_multiplier;
	}

	void Entity::setDamageMultiplier(const float multiplier)
	{
		_movement_state->damage_multiplier = multiplier;
	}

	float Entity::getDamageMultiplier() const
	{
		return _movement_state->damage_multiplier;
	}

	bool Entity::isMoving() const
	{
		return _movement_state->is_moving;
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
			_movement_state->velocity = {0.0f, 0.0f};
			_movement_state->is_moving = false;
			return;
		}

		_movement_state->target.x = x;
		_movement_state->target.y = y;
		_movement_state->is_moving = hasMovementTarget(_movement_state->position, {_movement_state->target.x, _movement_state->target.y});
	}

	void Entity::stopMovement()
	{
		_movement_state->velocity = {0.0f, 0.0f};
		_movement_state->is_moving = false;
	}

	void Entity::setMovementTarget(const float x, const float y)
	{
		_movement_state->target.x = x;
		_movement_state->target.y = y;
	}

	void Entity::tickPathRecalcTimer(const float dt)
	{
		_movement_state->path_recalc_timer -= dt;
	}

	bool Entity::isPathRecalcDue() const
	{
		return _movement_state->path_recalc_timer <= 0.0f;
	}

	void Entity::resetPathRecalcTimer(const float interval)
	{
		_movement_state->path_recalc_timer = interval;
	}

	void Entity::clearPathRecalcTimer()
	{
		_movement_state->path_recalc_timer = 0.0f;
	}

	void Entity::updateMovement(const float dt)
	{
		if (isMovementRooted()) {
			_movement_state->velocity = {0.0f, 0.0f};
			_movement_state->is_moving = false;
			return;
		}

		if (!_movement_state->is_moving) return;

		const float move_dist = _movement_state->movement_speed * _movement_state->speed_multiplier * dt;
		const MovementAdvanceResult movement = advanceMovementTowards(_movement_state->position, {_movement_state->target.x, _movement_state->target.y}, move_dist);
		if (movement.should_face_target)
			rotateTowards(_movement_state->target.x, _movement_state->target.y);

		_movement_state->position = movement.position;
		_movement_state->is_moving = movement.moving;
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
			const Vector2 target_position = getTargetPosition();
			moveTo(target_position.x, target_position.y);
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
