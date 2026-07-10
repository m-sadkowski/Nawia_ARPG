#include "RiftBinder.h"
#include "RiftBinderInternal.h"

#include <Map.h>
#include <SoundIds.h>

#include <cmath>

namespace Nawia::Entity {

	using namespace RiftBinderDetail;

	void RiftBinder::moveAwayFromTarget(const float dt)
	{
		const auto target = getTarget();
		if (!target)
			return;

		const Vector2 away = safeNormalize(Vector2Subtract(getCenter(), target->getCenter()), {1.0f, 0.0f});
		const Vector2 next = {
			getX() + away.x * MOVE_SPEED * getSpeedMultiplier() * 1.15f * dt,
			getY() + away.y * MOVE_SPEED * getSpeedMultiplier() * 1.15f * dt
		};

		if (!_map || _map->isWalkable(next.x, next.y)) {
			setPosition(next);
			faceTargetCenter();
			playWalk();
			return;
		}

		stopMoving();
		playIdle();
	}

	void RiftBinder::chaseToPreferredRange(const float dt)
	{
		const auto target = getTarget();
		if (!target)
			return;

		tickPathRecalcTimer(dt);
		if (isPathRecalcDue() || !isMoving()) {
			const Vector2 target_pos = target->getCenter();
			const Vector2 from_target = safeNormalize(Vector2Subtract(getCenter(), target_pos), {-1.0f, 0.0f});
			const Vector2 preferred = {
				target_pos.x + from_target.x * PREFERRED_DISTANCE,
				target_pos.y + from_target.y * PREFERRED_DISTANCE
			};
			const Vector2 walkable = findWalkableNearby(preferred, target_pos);
			moveTo(walkable.x, walkable.y);
			resetPathRecalcTimer(DEFAULT_PATH_RECALC_INTERVAL);
		}

		updateMovement(dt);
		if (isMoving())
			playWalk();
		else
			playIdle();
	}

	void RiftBinder::stopMoving()
	{
		stopMotion();
	}

	void RiftBinder::playIdle()
	{
		setAnimationSpeed(1.0f);
		playAnimation("idle");
	}

	void RiftBinder::playWalk()
	{
		setAnimationSpeed(_shield_active ? 1.22f : 1.0f);
		playAnimation("walk");
	}

	void RiftBinder::onDeathStarted()
	{
		clearCastTelemetry();
		stopMoving();
		playSoundEffect(Audio::SoundId::DevilDeath, 0.9f, true, 0.82f);
	}

	Vector2 RiftBinder::findWalkableNearby(const Vector2 preferred, const Vector2 fallback) const
	{
		if (!_map)
			return preferred;

		if (isReachableWalkable(fallback, preferred))
			return preferred;

		for (const float radius : {0.7f, 1.4f, 2.2f, 3.0f, 4.0f}) {
			const float angle_offset = randomFloat(0.0f, 360.0f) * DEG2RAD;
			for (int i = 0; i < 12; ++i) {
				const float angle = angle_offset + (static_cast<float>(i) / 12.0f) * 2.0f * PI;
				const Vector2 candidate = {
					preferred.x + std::cos(angle) * radius,
					preferred.y + std::sin(angle) * radius
				};

				if (isReachableWalkable(fallback, candidate))
					return candidate;
			}
		}

		if (_map->getNavMesh().isReady()) {
			const Vector3 snapped = _map->getNavMesh().getClosestWalkablePosition({preferred.x, getAltitude(), preferred.y});
			const Vector2 snapped_position = {snapped.x, snapped.z};
			if (isReachableWalkable(fallback, snapped_position))
				return snapped_position;
		}

		if (isReachableWalkable(fallback, fallback))
			return fallback;

		return fallback;
	}

	bool RiftBinder::isReachableWalkable(const Vector2 from, const Vector2 position) const
	{
		if (!_map)
			return true;

		if (!_map->isWalkable(position.x, position.y))
			return false;

		if (!_map->getNavMesh().isReady())
			return true;

		const auto path = _map->findPath(
			{from.x, getAltitude(), from.y},
			{position.x, getAltitude(), position.y});
		return !path.empty();
	}

	Vector2 RiftBinder::findTeleportDestination() const
	{
		const Vector2 center = targetCenterOrSelf();
		const Vector2 fallback = findWalkableNearby(getCenter(), getCenter());

		for (int i = 0; i < 18; ++i) {
			const float angle = randomFloat(0.0f, 360.0f) * DEG2RAD;
			const float radius = randomFloat(BLINK_MIN_RADIUS, BLINK_MAX_RADIUS);
			const Vector2 candidate = {
				center.x + std::cos(angle) * radius,
				center.y + std::sin(angle) * radius
			};
			const Vector2 walkable = findWalkableNearby(candidate, fallback);
			if (Vector2DistanceSqr(walkable, center) >= MIN_DISTANCE * MIN_DISTANCE)
				return walkable;
		}

		return fallback;
	}

	Vector2 RiftBinder::targetCenterOrSelf() const
	{
		if (const auto target = getTarget())
			return target->getCenter();

		return getCenter();
	}

} // namespace Nawia::Entity
