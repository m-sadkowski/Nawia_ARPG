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
		const std::string initial_loc = getCurrentLocationName();
		_spawn_manager.loadFromJson(path, engine, _map.get(), initial_loc);

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

	void Level::changeLocation(Core::Engine* engine, const std::string& location_name) {
		const auto locs = getLocations();
		for (size_t i = 0; i < locs.size(); ++i) {
			if (locs[i] == location_name) {
				_current_location_index = i;
				
				Core::Logger::debugLog("Level: Zmiana lokacji na " + location_name);

				// Update dormant states (freeze old location, wake up immediate new location)
				_spawn_manager.updateLocationChange(location_name);

				// Teleport player to new location spawn
				if (!engine)
					return;

				if (auto player = engine->getPlayer()) {
					Vector2 spawn_pos = {0.0f, 0.0f};
					if (_spawn_manager.getPlayerSpawn(location_name, spawn_pos)) {
						player->setX(spawn_pos.x);
						player->setY(spawn_pos.y);
						player->setRespawnPoint(spawn_pos);
						player->stop();
						_spawn_manager.update(spawn_pos, location_name);
					}
				}
				return;
			}
		}
		Core::Logger::errorLog("Level: Proba zmiany na nieznana lokacje " + location_name);
	}

} // namespace Nawia::World
