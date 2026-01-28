#include "EnemyInterface.h"
#include "Map.h"
#include "Collider.h"
#include <cmath>

#include <MathUtils.h>
#include <Constants.h>
#include <raymath.h>

namespace Nawia::Entity {

	EnemyInterface::EnemyInterface(const std::string& name, const float x, const float y, 
									const std::shared_ptr<Texture2D>& texture, const int max_hp, Core::Map* map)
		: Entity(name, x, y, texture, max_hp), _is_moving(false), _map(map), _target_x(x), _target_y(y) {
		_type = EntityType::Enemy;
	}

	void EnemyInterface::moveTo(const float x, const float y)
	{
		const Vector2 center = getCenter();
		const float offset_x = center.x - getX();
		const float offset_y = center.y - getY();

		if (!_map->isWalkable(x, y))
			return;

		_target_x = x - offset_x;
		_target_y = y - offset_y;

		const Vector2 start_world = { center.x, center.y };
		const Vector2 end_world = { x, y };

		_path = _map->findPath(start_world, end_world);
		
		if (_path.empty())
		{
			// Fallback: if very close, move directly
			const float dx = _target_x - getX();
			const float dy = _target_y - getY();
			if (dx * dx + dy * dy > 0.001f)
				_is_moving = true;
			else
				_is_moving = false;
			
		} 
		else 
		{
			_is_moving = true;
		}
	}

	void EnemyInterface::updateMovement(const float dt)
	{
		if (!_is_moving) return;

		Vector2 current_target_world;
		const Vector2 center = getCenter();
		
		if (!_path.empty()) 
		{
			current_target_world = _path.front();
		} 
		else 
		{
			const float offset_x = center.x - getX();
			const float offset_y = center.y - getY();
			current_target_world.x = _target_x + offset_x;
			current_target_world.y = _target_y + offset_y;
		}

		const float dx = current_target_world.x - center.x;
		const float dy = current_target_world.y - center.y;
		const float distance_sq = dx * dx + dy * dy;
		const float distance = std::sqrt(distance_sq);

		if (distance_sq > 0.001f)
			rotateTowardsCenter(current_target_world.x, current_target_world.y);

		if (distance < 0.1f * (_movement_speed / 4.0f)) 
		{
			if (!_path.empty()) 
			{
				_path.erase(_path.begin());
			} 
			else 
			{
				_is_moving = false;
				_pos.x = _target_x;
				_pos.y = _target_y;
				return;
			}
		}

		const float speed = _movement_speed;
		const float move_dist = speed * dt;

		if (move_dist >= distance && _path.empty()) {
			_pos.x = _target_x;
			_pos.y = _target_y;
			_is_moving = false;
		} 
		else 
		{
			// Validate movement against walkability
			const Vector2 direction = { dx / distance, dy / distance };
			const Vector2 validated_move = getValidatedMovement(center, direction, speed, dt);
			
			// If completely blocked, stop moving to force path recalculation
			if (std::abs(validated_move.x) < 0.0001f && std::abs(validated_move.y) < 0.0001f)
			{
				_is_moving = false;
				_path.clear();  // Clear invalid path
				return;
			}
			
			_pos.x += validated_move.x;
			_pos.y += validated_move.y;
		}
	}

	Vector2 EnemyInterface::getValidatedMovement(const Vector2 current_pos, const Vector2 direction, const float speed, const float dt) const
	{
		if (!_map) return { direction.x * speed * dt, direction.y * speed * dt };
		
		const float move_x = direction.x * speed * dt;
		const float move_y = direction.y * speed * dt;
		
		// Try full movement
		const float new_x = current_pos.x + move_x;
		const float new_y = current_pos.y + move_y;
		
		if (_map->isWalkable(new_x, new_y))
			return { move_x, move_y };
		
		// Try X-only movement (sliding along Y wall)
		if (std::abs(move_x) > 0.0001f && _map->isWalkable(current_pos.x + move_x, current_pos.y))
			return { move_x, 0.0f };
		
		// Try Y-only movement (sliding along X wall)
		if (std::abs(move_y) > 0.0001f && _map->isWalkable(current_pos.x, current_pos.y + move_y))
			return { 0.0f, move_y };
		
		// Completely blocked
		return { 0.0f, 0.0f };
	}

	// =============================================================================
	// Target Tracking Helpers
	// =============================================================================

	float EnemyInterface::getDistanceToTarget() const
	{
		const auto target = _target.lock();
		if (!target) return std::numeric_limits<float>::max();
		
		const Vector2 my_pos = getCollider() ? getCollider()->getPosition() : _pos;
		const Vector2 target_pos = target->getCollider() ? 
			target->getCollider()->getPosition() : target->getCenter();
		
		return Vector2Distance(my_pos, target_pos);
	}

	Vector2 EnemyInterface::getTargetPosition() const
	{
		const auto target = _target.lock();
		if (!target) return _pos;
		
		return target->getCollider() ? 
			target->getCollider()->getPosition() : target->getCenter();
	}

	bool EnemyInterface::hasValidTarget() const
	{
		const auto target = _target.lock();
		return target && !target->isDead();
	}

	void EnemyInterface::chaseTarget(const float dt, const float path_recalc_interval)
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