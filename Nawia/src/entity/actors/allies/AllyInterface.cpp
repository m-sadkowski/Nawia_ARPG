#include "AllyInterface.h"
#include "Map.h"
#include "Collider.h"

#include <cmath>
#include <raymath.h>

namespace Nawia::Entity {

	void AllyInterface::moveTo(const float x, const float y)
	{
		_target_x = x;
		_target_y = y;

		const float dx = _target_x - getX();
		const float dy = _target_y - getY();
		
		if (dx * dx + dy * dy > 0.001f)
			_is_moving = true;
		else
			_is_moving = false;
	}

	void AllyInterface::updateMovement(const float dt)
	{
		if (!_is_moving) return;

		const float dx = _target_x - getX();
		const float dy = _target_y - getY();
		const float distance = std::sqrt(dx * dx + dy * dy);

		if (distance > 0.001f)
			rotateTowards(_target_x, _target_y);

		const float speed = _movement_speed;
		const float move_dist = speed * dt;

		if (move_dist >= distance) 
		{
			_pos.x = _target_x;
			_pos.y = _target_y;
			_is_moving = false;
		} 
		else 
		{
			_pos.x += (dx / distance) * move_dist;
			_pos.y += (dy / distance) * move_dist;
		}
	}

	// =============================================================================
	// Target Tracking Helpers
	// =============================================================================

	float AllyInterface::getDistanceToTarget() const
	{
		const auto target = _target.lock();
		if (!target) return std::numeric_limits<float>::max();
		
		const Vector2 my_pos = getCenter();
		const Vector2 target_pos = target->getCenter();
		
		return Vector2Distance(my_pos, target_pos);
	}

	Vector2 AllyInterface::getTargetPosition() const
	{
		const auto target = _target.lock();
		if (!target) return _pos;
		
		return target->getCenter();
	}

	bool AllyInterface::hasValidTarget() const
	{
		const auto target = _target.lock();
		return target && !target->isDead();
	}

	void AllyInterface::chaseTarget(const float dt, const float path_recalc_interval)
	{
		if (!hasValidTarget()) return;
		
		_path_recalc_timer -= dt;
		
		if (_path_recalc_timer <= 0.0f || !_is_moving)
		{
			const Vector2 target_pos = getTargetPosition();
			moveTo(target_pos.x, target_pos.y);
			_path_recalc_timer = path_recalc_interval;
		}
		
		updateMovement(dt);
	}

} // namespace Nawia::Entity
