#include "LocationJsonLoader.h"

#include <Logger.h>

#include <fstream>

namespace Nawia::World {

	namespace {

		std::string toPathString(const std::filesystem::path& path) {
			return path.generic_string();
		}

		std::string displayNameFromStem(std::string stem) {
			for (char& character : stem) {
				if (character == '_' || character == '-')
					character = ' ';
			}
			return stem;
		}

		bool readJsonFile(const std::filesystem::path& path, nlohmann::json& output) {
			std::ifstream file(path);
			if (!file.is_open()) {
				Core::Logger::errorLog("LocationJsonLoader: nie mozna otworzyc pliku: " + toPathString(path));
				return false;
			}

			try {
				file >> output;
			} catch (const nlohmann::json::parse_error& error) {
				Core::Logger::errorLog(
					"LocationJsonLoader: blad parsowania " + toPathString(path) + ": " + std::string(error.what())
				);
				return false;
			}

			return true;
		}

		Vector2 parseVector2(const nlohmann::json& data, const Vector2 fallback = {0.0f, 0.0f}) {
			if (!data.is_object())
				return fallback;

			return {
				data.value("x", fallback.x),
				data.value("y", fallback.y),
			};
		}

		Vector3 parseVector3(const nlohmann::json& data, const Vector3 fallback = {0.0f, 0.0f, 0.0f}) {
			if (!data.is_object())
				return fallback;

			return {
				data.value("x", fallback.x),
				data.value("y", fallback.y),
				data.value("z", fallback.z),
			};
		}

		std::filesystem::path resolveObjectsPath(
			const std::filesystem::path& location_path,
			const nlohmann::json& location_data
		) {
			const std::filesystem::path directory = location_path.parent_path();
			if (location_data.contains("objects_file") && location_data["objects_file"].is_string())
				return directory / location_data["objects_file"].get<std::string>();

			return directory / ("objects_" + location_path.stem().string() + ".json");
		}

		void loadEntitiesFromObjectsFile(LocationDefinition& location) {
			if (location.objects_path.empty() || !std::filesystem::exists(location.objects_path))
				return;

			nlohmann::json objects_data;
			if (!readJsonFile(location.objects_path, objects_data))
				return;

			const nlohmann::json* entities = nullptr;
			if (objects_data.is_array()) {
				entities = &objects_data;
			} else if (objects_data.is_object() &&
					   objects_data.contains("entities") &&
					   objects_data["entities"].is_array()) {
				entities = &objects_data["entities"];
			}

			if (!entities)
				return;

			for (const auto& entry : *entities) {
				if (!entry.is_object())
					continue;

				nlohmann::json entity_data = entry;
				entity_data["location"] = location.name;
				location.entities.push_back(std::move(entity_data));
			}
		}

	} // namespace

	bool LocationJsonLoader::loadLocation(
		const std::filesystem::path& location_path,
		LocationDefinition& out_location
	) {
		nlohmann::json data;
		if (!readJsonFile(location_path, data))
			return false;

		LocationDefinition location;
		location.source_path = location_path;
		location.name = data.value("name", displayNameFromStem(location_path.stem().string()));
		location.objects_path = resolveObjectsPath(location_path, data);

		if (data.contains("map") && data["map"].is_object()) {
			const auto& map_data = data["map"];
			location.map.model = map_data.value("model", location.map.model);
			location.map.scale = map_data.value("scale", location.map.scale);
			location.map.offset = parseVector3(map_data.value("offset", nlohmann::json::object()));
			location.map.rotation = parseVector3(map_data.value("rotation", nlohmann::json::object()));
		}

		if (data.contains("player_spawn") && data["player_spawn"].is_object()) {
			location.player_spawn = parseVector2(data["player_spawn"]);
			location.has_player_spawn = true;
		}

		if (data.contains("navmesh") && data["navmesh"].is_object()) {
			const auto min_height_it = data["navmesh"].find("min_walkable_height");
			if (min_height_it != data["navmesh"].end() && min_height_it->is_number()) {
				location.navmesh_min_walkable_height = min_height_it->get<float>();
				location.has_navmesh_min_walkable_height = true;
			}
		}

		loadEntitiesFromObjectsFile(location);

		out_location = std::move(location);
		return true;
	}

	nlohmann::json LocationJsonLoader::buildSpawnRoot(const LocationDefinition& location) {
		nlohmann::json root;

		if (location.has_player_spawn) {
			root["player_spawn"][location.name] = {
				{"x", location.player_spawn.x},
				{"y", location.player_spawn.y},
			};
		}

		root["entities"] = nlohmann::json::array();
		for (auto entity_data : location.entities) {
			entity_data["location"] = location.name;
			root["entities"].push_back(std::move(entity_data));
		}

		return root;
	}

} // namespace Nawia::World
