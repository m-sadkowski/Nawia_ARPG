#include "Level.h"

#include <Engine.h>
#include <Logger.h>

namespace Nawia::World {

	void Level::onExit(Core::Engine* engine) {
		if (engine)
			engine->getEntityManager().clearNonPlayerEntities();
		_spawn_manager.reset();
	}

	void Level::update(Core::Engine* engine, float dt) {
		if (!engine) return;

		const auto player = engine->getPlayer();
		if (!player) return;

		const Vector2 player_pos = {player->getX(), player->getY()};
		const std::string current_loc = getCurrentLocationName();

		// Lightweight: just distance checks + dormant toggle
		_spawn_manager.update(player_pos, current_loc);
	}

	void Level::loadSpawns(Core::Engine* engine) {
		const std::string path = getSpawnFilePath();
		if (path.empty()) return;

		_spawn_manager.reset();
		_spawn_manager.loadFromJson(path, engine, _map.get());

		// Set player position from JSON if available
		if (engine) {
			const auto player = engine->getPlayer();
			if (player) {
				const std::string loc = getCurrentLocationName();
				Vector2 spawn_pos = {0.0f, 0.0f};
				if (_spawn_manager.getPlayerSpawn(loc, spawn_pos)) {
					player->respawn();
					player->setX(spawn_pos.x);
					player->setY(spawn_pos.y);
					player->setRespawnPoint(spawn_pos);
					player->stop();
				}
			}
		}

		_current_location_index = 0;
	}

} // namespace Nawia::World
