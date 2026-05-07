#include "Level.h"

#include <Engine.h>
#include <Logger.h>
#include <Map.h>
#include <Player.h>

namespace Nawia::World {

	Level::~Level() = default;

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
		const std::string current_location = getCurrentLocationName();

		// Lekka aktualizacja: tylko dystans do gracza i przelaczanie uspionych encji.
		_spawn_manager.update(player_pos, current_location);
	}

	std::string Level::getCurrentLocationName() const {
		const auto locations = getLocations();
		if (_current_location_index < locations.size()) {
			return locations[_current_location_index];
		}
		return locations.empty() ? "" : locations[0];
	}

	void Level::loadSpawns(Core::Engine* engine) {
		const std::string path = getSpawnFilePath();
		if (path.empty()) return;

		_spawn_manager.reset();
		const std::string initial_location = getCurrentLocationName();
		_spawn_manager.loadFromJson(path, engine, _map.get(), initial_location);

		// Ustawia gracza na pozycji startowej z JSON, jesli lokacja ja definiuje.
		if (engine) {
			const auto player = engine->getPlayer();
			if (player) {
				const std::string location = getCurrentLocationName();
				Vector2 spawn_pos = {0.0f, 0.0f};
				if (_spawn_manager.getPlayerSpawn(location, spawn_pos)) {
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
		const auto locations = getLocations();
		for (size_t i = 0; i < locations.size(); ++i) {
			if (locations[i] == location_name) {
				_current_location_index = i;
				
				Core::Logger::debugLog("Level: Zmiana lokacji na " + location_name);

				// Zamraza stare encje i budzi natychmiastowe spawny nowej lokacji.
				_spawn_manager.updateLocationChange(location_name);

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
