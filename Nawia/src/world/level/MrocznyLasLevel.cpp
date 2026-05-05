#include "MrocznyLasLevel.h"

#include <Map.h>
#include <Engine.h>
#include <Logger.h>

namespace Nawia::World {

	namespace {
		constexpr const char* MAP_FILE = "forest.glb";
		constexpr float MAP_SCALE = 2.0f;
	}

	void MrocznyLasLevel::onEnter(Core::Engine* engine) {
		Core::Logger::debugLog("Ladowanie poziomu Mroczny Las...");

		_map = std::make_unique<Core::Map>(engine->getResourceManager());
		_map->loadMap(MAP_FILE, MAP_SCALE);

		engine->getEntityManager().clearNonPlayerEntities();

		loadSpawns(engine);
	}

} // namespace Nawia::World
