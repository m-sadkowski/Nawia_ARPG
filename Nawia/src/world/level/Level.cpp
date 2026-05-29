#include "Level.h"

#include <AssetLoadManifest.h>
#include <Engine.h>
#include <Entity.h>
#include <LocationJsonLoader.h>
#include <Logger.h>
#include <Map.h>
#include <Player.h>

#include <json.hpp>

#include <algorithm>

namespace Nawia::World {

	namespace {

		bool isPlaceholderMapName(const std::string& model_name) {
			return model_name.empty() || model_name == "placeholder";
		}

		std::vector<nlohmann::json> collectLocationEntities(const LocationDefinition& location) {
			std::vector<nlohmann::json> entities;
			entities.reserve(location.entities.size());

			for (auto entity_data : location.entities) {
				entity_data["location"] = location.name;
				entities.push_back(std::move(entity_data));
			}

			return entities;
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

		return {};
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

	void Level::setPreparedLocationDefinitions(
		std::vector<LocationDefinition> definitions,
		const std::string& initial_location
	) {
		_location_definitions = std::move(definitions);
		_uses_location_files = true;
		_current_location_index = 0;

		if (_location_definitions.empty()) {
			Core::Logger::errorLog("Level: brak przygotowanych definicji lokacji");
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
	}

	void Level::activatePreparedLocations(Core::Engine* engine) {
		if (!engine || _location_definitions.empty())
			return;

		preloadLocationMapModels();
		loadLocationDefinition(engine, _current_location_index, true, false);
		rebuildLocationEntityPool(engine);
	}

	void Level::loadLocations(
		Core::Engine* engine,
		const std::vector<LevelLocationFile>& location_files,
		const std::string& initial_location
	) {
		std::vector<LocationDefinition> definitions;
		Core::AssetLoadManifest::buildForLocationFiles(location_files, definitions);
		setPreparedLocationDefinitions(std::move(definitions), initial_location);
		activatePreparedLocations(engine);
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

		_map->setNavMeshBlockers(location.navmesh_blockers);

		if (location.has_navmesh_min_walkable_height)
			_map->setNavMeshMinWalkableHeight(location.navmesh_min_walkable_height);

		engine->setGameplayCameraZoom(location.has_camera_zoom ? location.camera_zoom : 0.75f);

		if (reload_entities) {
			engine->getEntityManager().clearNonPlayerEntities();
			_spawn_manager.reset();
			_spawn_manager.loadEntities(
				collectLocationEntities(location),
				engine,
				_map.get(),
				location.name,
				location.source_path.generic_string()
			);
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

		Entity::Entity::setSharedResourceManager(&engine->getResourceManager());

		std::vector<nlohmann::json> entities;

		for (const auto& location : _location_definitions) {
			for (auto entity_data : location.entities) {
				entity_data["location"] = location.name;
				entities.push_back(std::move(entity_data));
			}
		}

		engine->getBossManager().clearPreloadedBosses();
		engine->getEntityManager().clearNonPlayerEntities();
		_spawn_manager.reset();
		_spawn_manager.loadEntities(entities, engine, _map.get(), getCurrentLocationName(), "all location json files");

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

		Core::Logger::errorLog("Level: ten poziom nie korzysta z plikow lokacji: " + location_name);
	}

} // namespace Nawia::World
