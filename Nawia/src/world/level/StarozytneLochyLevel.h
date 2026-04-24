#pragma once

#include "Level.h"

namespace Nawia::World {

	/**
	 * @class StarozytneLochyLevel
	 * @brief Ancient dungeon level with an arena sub-location.
	 *
	 * Locations:
	 *  - Lochy (dungeon corridors — starting area)
	 *  - Arena (combat arena — boss/wave area)
	 *
	 * Currently uses a placeholder map (demo_map/inferno.glb).
	 */
	class StarozytneLochyLevel : public Level {
	public:
		void onEnter(Core::Engine* engine) override;

		[[nodiscard]] std::string getName() const override { return "Starozytne Lochy"; }
		[[nodiscard]] std::vector<std::string> getLocations() const override {
			return {"Lochy", "Arena"};
		}
	};

} // namespace Nawia::World
