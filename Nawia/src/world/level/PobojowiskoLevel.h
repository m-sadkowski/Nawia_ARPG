#pragma once

#include "Level.h"

namespace Nawia::World {

	/**
	 * @class PobojowiskoLevel
	 * @brief Battlefield level with abandoned village sub-location.
	 *
	 * Locations:
	 *  - Pole Pobitewne (open battlefield — starting area)
	 *  - Opuszczona Wioska (abandoned village — exploration/loot area)
	 *
	 * Currently uses a placeholder map (demo_map/inferno.glb).
	 */
	class PobojowiskoLevel : public Level {
	public:
		void onEnter(Core::Engine* engine) override;

		[[nodiscard]] std::string getName() const override { return "Pobojowisko"; }
		[[nodiscard]] std::vector<std::string> getLocations() const override {
			return {"Pole Pobitewne", "Opuszczona Wioska"};
		}
	};

} // namespace Nawia::World
