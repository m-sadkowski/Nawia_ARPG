#include "Level.h"

#include <Engine.h>
#include <LocationJsonLoader.h>
#include <Logger.h>
#include <Map.h>
#include <Player.h>

#include <json.hpp>

#include <algorithm>
#include <fstream>

namespace Nawia::World {

	namespace {

		bool isPlaceholderMapName(const std::string& model_name) {
			return model_name.empty() || model_name == "placeholder";
		}

	} // namespace

	Level::~Level() = default;

	void Level::onExit(Core::Engine* engine) {
		if (engine)
			engine->getEntityManager().clearNonPlayerEntities();
		_spawn_manager.reset();
	}

	std::vector<std::string> Level::getLocations() const {
		if (!_location_definitions.empty()) {
			std::vector<std::string> locations;
			locations.reserve(_location_definitions.size());
			for (const auto& location : _location_definitions)
				locations.push_back(location.name);
			return locations;
		}

		return {"Domyslna"};
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
		if (_uses_location_files && _current_location_index < _location_definitions.size())
			return _location_definitions[_current_location_index].name;

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

	void Level::loadLocations(
		Core::Engine* engine,
		const std::vector<LevelLocationFile>& location_files,
		const std::string& initial_location
	) {
		_location_definitions.clear();
		_uses_location_files = true;
		_current_location_index = 0;

		for (const auto& location_file : location_files) {
			LocationDefinition definition;
			if (!LocationJsonLoader::loadLocation(location_file.path, definition))
				continue;

			if (!location_file.name.empty())
				definition.name = location_file.name;

			_location_definitions.push_back(std::move(definition));
		}

		if (_location_definitions.empty()) {
			Core::Logger::errorLog("Level: nie udalo sie wczytac zadnej lokacji z JSON");
			return;
		}

		if (!initial_location.empty()) {
			const auto initial_it = std::ranges::find_if(_location_definitions, [&](const LocationDefinition& location) {
				return location.name == initial_location;
			});

			if (initial_it != _location_definitions.end())
				_current_location_index = static_cast<size_t>(std::distance(_location_definitions.begin(), initial_it));
			else
				Core::Logger::errorLog("Level: nie znaleziono lokacji startowej " + initial_location);
		}

		preloadLocationMapModels();
		loadLocationDefinition(engine, _current_location_index, true, false);
		rebuildLocationEntityPool(engine);
	}

	bool Level::loadLocationDefinition(
		Core::Engine* engine,
		const size_t location_index,
		const bool move_player_to_spawn,
		const bool reload_entities
	) {
		if (!engine || location_index >= _location_definitions.size())
			return false;

		const auto& location = _location_definitions[location_index];

		if (!_map)
			_map = std::make_unique<Core::Map>(engine->getResourceManager());

		if (isPlaceholderMapName(location.map.model))
			_map->loadPlaceholder();
		else
			_map->loadMap(location.map.model, location.map.scale, location.map.offset, location.map.rotation);

		if (location.has_navmesh_min_walkable_height)
			_map->setNavMeshMinWalkableHeight(location.navmesh_min_walkable_height);

		if (reload_entities) {
			engine->getEntityManager().clearNonPlayerEntities();
			_spawn_manager.reset();
			const nlohmann::json spawn_root = LocationJsonLoader::buildSpawnRoot(location);
			_spawn_manager.loadFromJsonData(spawn_root, engine, _map.get(), location.name, location.source_path.generic_string());
		} else {
			_spawn_manager.updateLocationChange(location.name, _map.get());
		}

		Vector2 active_player_pos = {0.0f, 0.0f};
		bool has_active_player_pos = false;
		if (move_player_to_spawn && location.has_player_spawn) {
			if (auto player = engine->getPlayer()) {
				Vector3 spawn_position = {location.player_spawn.x, player->getAltitude(), location.player_spawn.y};
				if (_map->getNavMesh().isReady())
					spawn_position = _map->getNavMesh().getClosestWalkablePosition({location.player_spawn.x, 0.0f, location.player_spawn.y});

				const Vector2 spawn_2d = {spawn_position.x, spawn_position.z};
				player->setX(spawn_2d.x);
				player->setY(spawn_2d.y);
				player->setAltitude(spawn_position.y);
				player->setRespawnPoint(spawn_2d);
				player->stop();
				active_player_pos = spawn_2d;
				has_active_player_pos = true;
			}
		}

		if (!has_active_player_pos) {
			if (const auto player = engine->getPlayer()) {
				active_player_pos = {player->getX(), player->getY()};
				has_active_player_pos = true;
			}
		}

		if (has_active_player_pos)
			_spawn_manager.update(active_player_pos, location.name);

		Core::Logger::debugLog("Level: zaladowano lokacje " + location.name + " z " + location.source_path.generic_string());
		return true;
	}

	void Level::rebuildLocationEntityPool(Core::Engine* engine) {
		if (!engine || !_map || _location_definitions.empty())
			return;

		nlohmann::json root;
		root["player_spawn"] = nlohmann::json::object();
		root["entities"] = nlohmann::json::array();

		for (const auto& location : _location_definitions) {
			if (location.has_player_spawn) {
				root["player_spawn"][location.name] = {
					{"x", location.player_spawn.x},
					{"y", location.player_spawn.y},
				};
			}

			for (auto entity_data : location.entities) {
				entity_data["location"] = location.name;
				root["entities"].push_back(std::move(entity_data));
			}
		}

		engine->getBossManager().clearPreloadedBosses();
		engine->getEntityManager().clearNonPlayerEntities();
		_spawn_manager.reset();
		_spawn_manager.loadFromJsonData(root, engine, _map.get(), getCurrentLocationName(), "all location json files");

		if (const auto player = engine->getPlayer())
			_spawn_manager.update({player->getX(), player->getY()}, getCurrentLocationName());
	}

	void Level::preloadLocationMapModels() const {
		for (const auto& location : _location_definitions) {
			if (!isPlaceholderMapName(location.map.model))
				Core::Map::preloadMapModel(location.map.model);
		}
	}

	void Level::prepareForRespawn(Core::Engine* engine) {
		if (!_uses_location_files || _location_definitions.empty() || !engine)
			return;

		loadLocationDefinition(engine, _current_location_index, false, false);
		rebuildLocationEntityPool(engine);
	}

	void Level::applyNavMeshSettingsFromJson(const std::string& location_name) {
		const std::string path = getSpawnFilePath();
		if (path.empty() || !_map) return;

		std::ifstream file(path);
		if (!file.is_open()) {
			Core::Logger::debugLog("Level: brak pliku ustawien navmesha: " + path);
			return;
		}

		nlohmann::json root;
		try {
			file >> root;
		} catch (const nlohmann::json::parse_error& error) {
			Core::Logger::errorLog("Level: blad parsowania ustawien navmesha: " + std::string(error.what()));
			return;
		}

		const auto navmesh_it = root.find("navmesh");
		if (navmesh_it == root.end() || !navmesh_it->is_object())
			return;

		const nlohmann::json* navmesh_settings = &(*navmesh_it);
		if (!location_name.empty()) {
			const auto location_it = navmesh_it->find(location_name);
			if (location_it != navmesh_it->end() && location_it->is_object())
				navmesh_settings = &(*location_it);
		}

		const nlohmann::json* min_height_value = nullptr;
		const auto min_height_it = navmesh_settings->find("min_walkable_height");
		if (min_height_it != navmesh_settings->end())
			min_height_value = &(*min_height_it);

		if (!min_height_value && navmesh_settings != &(*navmesh_it)) {
			const auto global_min_height_it = navmesh_it->find("min_walkable_height");
			if (global_min_height_it != navmesh_it->end())
				min_height_value = &(*global_min_height_it);
		}

		if (!min_height_value || !min_height_value->is_number())
			return;

		const float min_walkable_height = min_height_value->get<float>();
		_map->setNavMeshMinWalkableHeight(min_walkable_height);
		Core::Logger::debugLog("Level: zastosowano navmesh.min_walkable_height=" + std::to_string(min_walkable_height));
	}

	void Level::changeLocation(Core::Engine* engine, const std::string& location_name) {
		if (_uses_location_files) {
			const auto location_it = std::ranges::find_if(_location_definitions, [&](const LocationDefinition& location) {
				return location.name == location_name;
			});

			if (location_it == _location_definitions.end()) {
				Core::Logger::errorLog("Level: Proba zmiany na nieznana lokacje " + location_name);
				return;
			}

			_current_location_index = static_cast<size_t>(std::distance(_location_definitions.begin(), location_it));
			Core::Logger::debugLog("Level: Zmiana lokacji na " + location_name);
			loadLocationDefinition(engine, _current_location_index, true, false);
			return;
		}

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
