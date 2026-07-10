#include "FireRainHazard.h"

#include <raymath.h>

#include <algorithm>
#include <cmath>
#include <utility>

namespace Nawia::Entity {

	namespace {
		constexpr int DROP_COUNT = 24;
		constexpr float DROP_MIN_HEIGHT = 4.0f;
		constexpr float DROP_MAX_HEIGHT = 8.0f;
	}

	FireRainHazard::FireRainHazard(BossTelegraphHazardConfig config)
		: BossTelegraphHazard(std::move(config))
	{
		_drops.reserve(DROP_COUNT);
		for (int i = 0; i < DROP_COUNT; ++i) {
			const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
			const float distance = std::sqrt(static_cast<float>(GetRandomValue(0, 1000)) / 1000.0f) * getRadius();
			FireDrop drop;
			drop.offset = {std::cos(angle) * distance, std::sin(angle) * distance};
			drop.phase = static_cast<float>(GetRandomValue(0, 1000)) / 1000.0f;
			drop.height = DROP_MIN_HEIGHT + static_cast<float>(GetRandomValue(0, 1000)) / 1000.0f * (DROP_MAX_HEIGHT - DROP_MIN_HEIGHT);
			drop.size = 0.035f + static_cast<float>(GetRandomValue(0, 35)) / 1000.0f;
			_drops.push_back(drop);
		}
	}

	void FireRainHazard::update(const float dt)
	{
		BossTelegraphHazard::update(dt);
		_visual_time += std::max(0.0f, dt);
	}

	void FireRainHazard::render(const Camera3D& camera)
	{
		BossTelegraphHazard::render(camera);
		if (isDead() || isDormant())
			return;

		const Vector2 center = getCenter();
		const float active_elapsed = std::max(0.0f, getElapsedSeconds() - getWarningSeconds());
		const float warning_fraction = getWarningSeconds() <= 0.0f
			? 1.0f
			: std::clamp(getElapsedSeconds() / getWarningSeconds(), 0.0f, 1.0f);

		for (const auto& drop : _drops) {
			const float cycle = std::fmod(active_elapsed * 1.45f + drop.phase, 1.0f);
			const float x = center.x + drop.offset.x;
			const float z = center.y + drop.offset.y;

			if (isWarning()) {
				const float y = getAltitude() + drop.height * (0.85f + 0.15f * std::sin(_visual_time * 4.0f + drop.phase));
				DrawLine3D({x, y + 0.45f, z}, {x, y - 0.75f, z}, Fade(Color{255, 95, 30, 255}, 0.38f + warning_fraction * 0.30f));
				continue;
			}

			const float y = getAltitude() + 0.25f + (1.0f - cycle) * drop.height;
			const Color flame = Color{255, 78, 22, 255};
			DrawLine3D({x, y + 0.8f, z}, {x, y - 0.35f, z}, Fade(flame, 0.82f));

			if (cycle > 0.90f) {
				const float impact_alpha = (cycle - 0.90f) / 0.10f;
				DrawCircle3D(
					{x, getAltitude() + 0.12f, z},
					0.22f + impact_alpha * 0.34f,
					{1.0f, 0.0f, 0.0f},
					90.0f,
					Fade(Color{255, 128, 32, 255}, 0.75f * (1.0f - impact_alpha)));
			}
		}
	}

} // namespace Nawia::Entity
