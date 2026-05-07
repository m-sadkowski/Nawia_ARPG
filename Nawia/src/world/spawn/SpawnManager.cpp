#include "SpawnManager.h"

#include <Engine.h>
#include <Entity.h>
#include <EntityFactory.h>
#include <Logger.h>
#include <Map.h>

#include <json.hpp>

#include <cmath>
#include <fstream>
#include <utility>

using json = nlohmann::json;

namespace Nawia::World {

	bool SpawnManager::loadFromJson(
		const std::string& path,
		Core::Engine* engine,
		Core::Map* map,
		const std::string& initial_location
	) {
		std::ifstream file(path);
		if (!file.is_open()) {
			Core::Logger::errorLog("SpawnManager: nie mozna otworzyc pliku: " + path);
			return false;
		}

		json root;
		try {
			file >> root;
		} catch (const json::parse_error& e) {
			Core::Logger::errorLog("SpawnManager: blad parsowania JSON: " + std::string(e.what()));
			return false;
		}

		_spawn_points.clear();
		_player_spawns.clear();

		// Wczytuje pozycje startowe gracza dla poszczegolnych lokacji.
		if (root.contains("player_spawn")) {
			for (const auto& [location_name, position_data] : root["player_spawn"].items()) {
				const float x = position_data.value("x", 0.0f);
				const float y = position_data.value("y", 0.0f);
				_player_spawns[location_name] = {x, y};
			}
		}

		// Brak encji nie jest bledem; poziom moze miec tylko spawn gracza.
		if (!root.contains("entities") || !root["entities"].is_array()) {
			Core::Logger::debugLog("SpawnManager: brak tablicy 'entities' w pliku: " + path);
			return true;
		}

		for (const auto& entry : root["entities"]) {
			SpawnPoint spawn_point;

			spawn_point.location = entry.value("location", "");
			spawn_point.entity_type = entry.value("type", "");
			spawn_point.entity_data = entry;

			spawn_point.spawn_center.x = entry.value("x", 0.0f);
			spawn_point.spawn_center.y = entry.value("y", 0.0f);
			spawn_point.trigger_radius = entry.value("trigger_radius", 0.0f);
			spawn_point.spawn_radius = entry.value("spawn_radius", 0.0f);
			spawn_point.respawnable = entry.value("respawnable", false);
			spawn_point.respawn_cooldown = entry.value("respawn_cooldown", 0.0f);

			if (spawn_point.entity_type.empty()) {
				Core::Logger::errorLog("SpawnManager: spawn point bez 'type' - pominieto");
				continue;
			}

			json spawn_data = spawn_point.entity_data;

			// Losuje pozycje w promieniu spawnu, jesli JSON tego wymaga.
			if (spawn_point.spawn_radius > 0.0f) {
				const float angle = static_cast<float>(GetRandomValue(0, 360)) * DEG2RAD;
				const int max_distance = static_cast<int>(spawn_point.spawn_radius * 100);
				const float distance = static_cast<float>(GetRandomValue(0, max_distance)) / 100.0f;
				spawn_data["x"] = spawn_point.spawn_center.x + cosf(angle) * distance;
				spawn_data["y"] = spawn_point.spawn_center.y + sinf(angle) * distance;
			}

			auto entity = EntityFactory::create(spawn_point.entity_type, spawn_data, engine, map);
			if (!entity) {
				Core::Logger::errorLog("SpawnManager: nie udalo sie stworzyc: " + spawn_point.entity_type);
				continue;
			}

			if (map && map->getNavMesh().isReady()) {
				const Vector3 snapped_position = map->getNavMesh().getClosestWalkablePosition(
					{entity->getX(), entity->getAltitude(), entity->getY()});
				entity->setX(snapped_position.x);
				entity->setY(snapped_position.z);
				entity->setAltitude(snapped_position.y);
			}

			// Inne lokacje i spawny dystansowe startuja uspione.
			const bool is_current_location = (spawn_point.location == initial_location);
			const bool should_be_active = is_current_location && (spawn_point.trigger_radius <= 0.0f);

			entity->setDormant(!should_be_active);
			spawn_point.activated = should_be_active;
			spawn_point.entity = entity;
			engine->getEntityManager().addEntity(entity);

			_spawn_points.push_back(std::move(spawn_point));
		}

		Core::Logger::debugLog("SpawnManager: zaladowano i stworzono " +
			std::to_string(_spawn_points.size()) + " encji z " + path);
		return true;
	}

	void SpawnManager::update(const Vector2 player_pos, const std::string& current_location) {
		for (auto& spawn_point : _spawn_points) {
			if (spawn_point.location != current_location) continue;
			if (spawn_point.activated) continue;
			if (!spawn_point.entity) continue;

			if (spawn_point.trigger_radius > 0.0f) {
				const float dx = player_pos.x - spawn_point.spawn_center.x;
				const float dy = player_pos.y - spawn_point.spawn_center.y;
				const float distance_sq = dx * dx + dy * dy;
				const float trigger_sq = spawn_point.trigger_radius * spawn_point.trigger_radius;

				if (distance_sq > trigger_sq) continue;
			}

			spawn_point.entity->setDormant(false);
			spawn_point.activated = true;

			Core::Logger::debugLog("SpawnManager: aktywowano " + spawn_point.entity_type +
				" w lokacji " + spawn_point.location);
		}
	}

	void SpawnManager::updateLocationChange(const std::string& new_location) {
		for (auto& spawn_point : _spawn_points) {
			if (!spawn_point.entity) continue;

			if (spawn_point.location == new_location) {
				// Budzi natychmiastowe spawny oraz encje aktywowane przed opuszczeniem lokacji.
				if (spawn_point.trigger_radius <= 0.0f || spawn_point.activated) {
					spawn_point.entity->setDormant(false);
					spawn_point.activated = true;
				} else {
					// Encja poczeka na podejscie gracza.
					spawn_point.entity->setDormant(true);
				}
			} else {
				// Po opuszczeniu lokacji encja jest zamrazana, ale zachowuje stan aktywacji.
				spawn_point.entity->setDormant(true);
			}
		}
	}

	void SpawnManager::reset() {
		for (auto& spawn_point : _spawn_points) {
			spawn_point.reset();
		}
		_spawn_points.clear();
		_player_spawns.clear();
	}

	bool SpawnManager::getPlayerSpawn(const std::string& location_name, Vector2& out_pos) const {
		const auto spawn_it = _player_spawns.find(location_name);
		if (spawn_it != _player_spawns.end()) {
			out_pos = spawn_it->second;
			return true;
		}
		return false;
	}

} // namespace Nawia::World
