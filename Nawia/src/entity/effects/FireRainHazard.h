#pragma once

#include <BossTelegraphHazard.h>

#include <vector>

namespace Nawia::Entity {

	/**
	 * @brief Fire-rain hazard with falling visual projectiles above the telegraph.
	 */
	class FireRainHazard : public BossTelegraphHazard {
	public:
		explicit FireRainHazard(BossTelegraphHazardConfig config);

		void update(float dt) override;
		void render(const Camera3D& camera) override;

	private:
		struct FireDrop {
			Vector2 offset = {0.0f, 0.0f};
			float phase = 0.0f;
			float height = 5.0f;
			float size = 0.055f;
		};

		std::vector<FireDrop> _drops;
		float _visual_time = 0.0f;
	};

} // namespace Nawia::Entity
