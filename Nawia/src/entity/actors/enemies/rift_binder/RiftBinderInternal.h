#pragma once

#include "RiftBinder.h"

#include <raymath.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace Nawia::Entity::RiftBinderDetail {

	constexpr const char* DRAGON_MODEL = "assets/models/actors/dragon/dragon.glb";
	constexpr int DRAGON_ANIM_DEATH = 0;
	constexpr int DRAGON_ANIM_FAST_FLYING = 1;
	constexpr int DRAGON_ANIM_FLYING_IDLE = 2;
	constexpr int DRAGON_ANIM_HEADBUTT = 3;
	constexpr int DRAGON_ANIM_HIT_REACT = 4;
	constexpr int DRAGON_ANIM_NO = 5;
	constexpr int DRAGON_ANIM_PUNCH = 6;
	constexpr const char* STONE_PROJECTILE_MODEL = "assets/models/fireball.glb";
	constexpr float STONE_PROJECTILE_MODEL_SCALE = 0.3f;
	constexpr float MIN_DIRECTION_LENGTH_SQ = 0.0001f;
	constexpr Color STONE_PROJECTILE_TINT = {125, 125, 125, 255};
	constexpr std::array<int, 4> TOTEMS_BY_STAGE = {3, 4, 5, 7};
	constexpr std::array<float, 4> STAGE_THRESHOLDS = {1.0f, 0.75f, 0.50f, 0.25f};
	constexpr std::array<float, 4> FIRE_RAIN_STAGE_BASE_COOLDOWNS = {10.0f, 8.0f, 6.5f, 5.5f};
	constexpr std::array<float, 4> FIRE_RAIN_STAGE_MIN_COOLDOWNS = {8.5f, 6.5f, 5.5f, 4.0f};

	inline Vector2 safeNormalize(const Vector2 value, const Vector2 fallback)
	{
		if (Vector2LengthSqr(value) <= MIN_DIRECTION_LENGTH_SQ)
			return fallback;

		return Vector2Normalize(value);
	}

	inline float randomFloat(const float min_value, const float max_value)
	{
		const int min_scaled = static_cast<int>(std::round(min_value * 100.0f));
		const int max_scaled = static_cast<int>(std::round(max_value * 100.0f));
		return static_cast<float>(GetRandomValue(min_scaled, max_scaled)) / 100.0f;
	}

	inline int stageThresholdHp(const int max_hp, const int stage_index)
	{
		if (stage_index < 0 || stage_index >= static_cast<int>(STAGE_THRESHOLDS.size()))
			return 0;

		return static_cast<int>(std::ceil(static_cast<float>(max_hp) * STAGE_THRESHOLDS[stage_index]));
	}

	inline bool isFarEnoughFrom(
		const std::vector<Vector2>& positions,
		const Vector2 candidate,
		const float min_distance)
	{
		const float min_distance_sq = min_distance * min_distance;
		return std::ranges::all_of(positions, [candidate, min_distance_sq](const Vector2 existing) {
			return Vector2DistanceSqr(existing, candidate) >= min_distance_sq;
		});
	}

} // namespace Nawia::Entity::RiftBinderDetail
