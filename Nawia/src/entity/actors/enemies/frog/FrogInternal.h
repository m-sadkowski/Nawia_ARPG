#pragma once

namespace Nawia::Entity::FrogTuning {

	inline constexpr const char* MODEL_PATH = "assets/models/actors/frog/frog.glb";

	inline constexpr float TONGUE_MIN_RANGE = 4.5f;
	inline constexpr float TONGUE_MAX_RANGE = 13.0f;
	inline constexpr float TONGUE_HALF_WIDTH = 1.15f;
	inline constexpr float TONGUE_MOUTH_FORWARD_OFFSET = 0.58f;
	inline constexpr float TONGUE_VISUAL_WIDTH_SCALE = 0.7f;
	inline constexpr float TONGUE_VISUAL_BASE_HALF_WIDTH = 0.24f * TONGUE_VISUAL_WIDTH_SCALE;
	inline constexpr float TONGUE_VISUAL_TIP_HALF_WIDTH = 0.52f * TONGUE_VISUAL_WIDTH_SCALE;
	inline constexpr float TONGUE_TELEGRAPH_BASE_HALF_WIDTH = TONGUE_HALF_WIDTH * TONGUE_VISUAL_WIDTH_SCALE;
	inline constexpr float TONGUE_TELEGRAPH_TIP_HALF_WIDTH =
		(TONGUE_HALF_WIDTH + TONGUE_MAX_RANGE * 0.035f) * TONGUE_VISUAL_WIDTH_SCALE;
	inline constexpr float TONGUE_WINDUP_TIME = 0.38f;
	inline constexpr float TONGUE_PULL_TIME = 0.34f;
	inline constexpr float TONGUE_RECOVER_TIME = 0.38f;
	inline constexpr float TONGUE_PULL_SPEED = 24.0f;
	inline constexpr float TONGUE_STOP_DISTANCE = 2.75f;
	inline constexpr int TONGUE_DAMAGE = 8;

	inline constexpr float SIDEHOP_DURATION = 0.48f;
	inline constexpr float SIDEHOP_DISTANCE = 4.25f;

	inline constexpr float TOXIC_POOL_WINDUP_TIME = 0.72f;
	inline constexpr float TOXIC_POOL_RADIUS = 2.65f;
	inline constexpr float TOXIC_POOL_DURATION = 5.2f;
	inline constexpr int TOXIC_POOL_DAMAGE = 7;

} // namespace Nawia::Entity::FrogTuning
