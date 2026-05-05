#include "PobojowiskoLevel.h"

#include <Map.h>
#include <Engine.h>
#include <Logger.h>

namespace Nawia::World {

	namespace {
		constexpr const char* MAP_FILE = "rocky-forest.glb";
		constexpr float MAP_SCALE = 1.5f;
	}

	void PobojowiskoLevel::onEnter(Core::Engine* engine) {
		Core::Logger::debugLog("Ladowanie poziomu Pobojowisko...");

		_map = std::make_unique<Core::Map>(engine->getResourceManager());
		_map->loadMap(MAP_FILE, MAP_SCALE);

		engine->getEntityManager().clearNonPlayerEntities();

		loadSpawns(engine);
	}

} // namespace Nawia::World
