#include "RiftBinder.h"
#include "RiftBinderInternal.h"

#include <RiftTotem.h>
#include <SoundIds.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace Nawia::Entity {

	using namespace RiftBinderDetail;

	void RiftBinder::updateTotemStage()
	{
		if (!_shield_active)
			return;

		if (livingTotemCount() > 0)
			return;

		_shield_active = false;
		_active_stage = -1;
		_action_cooldown_timer = 0.15f;
		playSoundEffect(Audio::SoundId::DevilDashHit, 0.45f, true, 0.75f);
	}

	void RiftBinder::startTotemStage(const int stage_index)
	{
		if (stage_index < 0 || stage_index >= static_cast<int>(TOTEMS_BY_STAGE.size()))
			return;

		_active_stage = stage_index;
		_next_stage_to_start = std::max(_next_stage_to_start, stage_index + 1);
		_shield_active = true;
		_totems.clear();
		spawnStageTotems(stage_index);
		_action_cooldown_timer = 0.20f;
		_stone_cooldown_timer = 0.20f;
		_fire_rain_cooldown_timer = std::min(_fire_rain_cooldown_timer, stage_index == 0 ? 2.0f : 0.9f);
		_blink_cooldown_timer = std::max(_blink_cooldown_timer, 8.0f);
		playSoundEffect(Audio::SoundId::DevilDash, 0.72f, true, 0.82f);
	}

	void RiftBinder::spawnStageTotems(const int stage_index)
	{
		std::shared_ptr<Entity> owner;
		try {
			owner = shared_from_this();
		} catch (const std::bad_weak_ptr&) {
			return;
		}

		const auto target = getTarget();
		const int count = TOTEMS_BY_STAGE[static_cast<size_t>(stage_index)];
		const Vector2 center = getCenter();
		const Vector2 target_pos = targetCenterOrSelf();
		const Vector2 forward = safeNormalize(Vector2Subtract(target_pos, center), {1.0f, 0.0f});
		const float base_angle = std::atan2(forward.y, forward.x) + randomFloat(-0.25f, 0.25f);
		const float angle_step = 2.0f * PI / static_cast<float>(count);
		std::vector<Vector2> spawned_positions;

		for (int i = 0; i < count; ++i) {
			Vector2 spawn_pos = findWalkableNearby(center, center);
			bool found_position = false;

			for (int attempt = 0; attempt < TOTEM_POSITION_ATTEMPTS; ++attempt) {
				const float angle = base_angle + static_cast<float>(i) * angle_step +
					randomFloat(-angle_step * 0.32f, angle_step * 0.32f);
				const float radius = TOTEM_RING_RADIUS +
					randomFloat(-TOTEM_RING_RADIUS_JITTER, TOTEM_RING_RADIUS_JITTER);
				const Vector2 direction = {std::cos(angle), std::sin(angle)};
				const Vector2 tangent = {-direction.y, direction.x};
				const float side_offset = randomFloat(-TOTEM_TANGENTIAL_JITTER, TOTEM_TANGENTIAL_JITTER);
				const Vector2 preferred = {
					center.x + direction.x * radius + tangent.x * side_offset,
					center.y + direction.y * radius + tangent.y * side_offset
				};
				const Vector2 candidate = findWalkableNearby(preferred, center);

				if (!isReachableWalkable(center, candidate))
					continue;

				const bool can_overlap = attempt >= TOTEM_POSITION_ATTEMPTS / 2;
				if (can_overlap || isFarEnoughFrom(spawned_positions, candidate, TOTEM_MIN_SEPARATION)) {
					spawn_pos = candidate;
					found_position = true;
					break;
				}
			}

			if (!found_position)
				spawn_pos = findWalkableNearby(center, center);

			spawned_positions.push_back(spawn_pos);
			auto totem = std::make_shared<RiftTotem>(
				spawn_pos.x,
				spawn_pos.y,
				_map,
				owner,
				target,
				stage_index);
			totem->setAltitude(getAltitude());
			totem->setAudioManager(getAudioManager());
			_totems.push_back(totem);
			addPendingSpawn(totem);
		}
	}

	int RiftBinder::livingTotemCount()
	{
		return static_cast<int>(liveTotems().size());
	}

	std::vector<std::shared_ptr<RiftTotem>> RiftBinder::liveTotems()
	{
		std::vector<std::shared_ptr<RiftTotem>> result;
		std::erase_if(_totems, [&result](const std::weak_ptr<RiftTotem>& weak_totem) {
			const auto totem = weak_totem.lock();
			if (!totem || totem->isDead() || totem->isDying())
				return true;

			result.push_back(totem);
			return false;
		});

		return result;
	}

	bool RiftBinder::canCastFireRain()
	{
		return _shield_active &&
			   _active_stage >= 0 &&
			   _active_stage < static_cast<int>(TOTEMS_BY_STAGE.size()) &&
			   livingTotemCount() > 0;
	}

	float RiftBinder::currentFireRainCooldown()
	{
		if (_active_stage < 0 || _active_stage >= static_cast<int>(TOTEMS_BY_STAGE.size()))
			return FIRE_RAIN_STAGE_BASE_COOLDOWNS.front();

		const int total_totems = TOTEMS_BY_STAGE[static_cast<size_t>(_active_stage)];
		const int living_totems = std::clamp(livingTotemCount(), 0, total_totems);
		const int destroyed_totems = std::clamp(total_totems - living_totems, 0, total_totems);
		const float base_cooldown = FIRE_RAIN_STAGE_BASE_COOLDOWNS[static_cast<size_t>(_active_stage)];
		const float min_cooldown = FIRE_RAIN_STAGE_MIN_COOLDOWNS[static_cast<size_t>(_active_stage)];
		const float step = total_totems > 0
			? (base_cooldown - min_cooldown) / static_cast<float>(total_totems)
			: 0.0f;

		return std::max(min_cooldown, base_cooldown - step * static_cast<float>(destroyed_totems));
	}

} // namespace Nawia::Entity
