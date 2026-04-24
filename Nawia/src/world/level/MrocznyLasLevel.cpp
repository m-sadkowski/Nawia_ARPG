#include "MrocznyLasLevel.h"

#include <Map.h>
#include <Engine.h>
#include <Logger.h>

namespace Nawia::World {

	void MrocznyLasLevel::onEnter(Core::Engine* engine) {
		Core::Logger::debugLog("Ladowanie poziomu Mroczny Las...");

		// Placeholder: use DevLevel map
		_map = std::make_unique<Core::Map>(engine->getResourceManager());
		_map->loadMap("demo_map/inferno.glb", 1.f, {0.0f, -20.0f, 0.0f}, {0, 180, 0});

		engine->getEntityManager().clearNonPlayerEntities();

		const auto player = engine->getPlayer();
		if (player) {
			player->respawn();
			player->setX(0.0f);
			player->setY(0.0f);
			player->setRespawnPoint({0.0f, 0.0f});
			player->stop();
		}

		_current_location_index = 0;
	}

} // namespace Nawia::World
