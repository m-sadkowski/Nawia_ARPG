#include "LocationJsonLoader.h"

#include <LocationJsonUtils.h>

#include <algorithm>

namespace Nawia::World {

	namespace {

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
			if (!LocationJsonUtils::readJsonFile(location.objects_path, objects_data, "LocationJsonLoader"))
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
				const std::string type = entity_data.value("type", "");
				const std::string category = entity_data.value("category", "");
				if (type == "nav_blocker" || category == "nav_blockers") {
					NavMeshBlocker blocker;
					blocker.center = {
						entity_data.value("x", 0.0f),
						entity_data.value("y", 0.0f)
					};
					blocker.width = entity_data.value("width", 4.0f);
					blocker.depth = entity_data.value("depth", 4.0f);
					blocker.height = entity_data.value("height", 0.0f);
					blocker.radius = entity_data.value("radius", std::max(blocker.width, blocker.depth) * 0.5f);
					const std::string shape = entity_data.value("shape", "box");
					blocker.shape = shape == "circle" ? NavMeshBlockerShape::Circle : NavMeshBlockerShape::Box;
					location.navmesh_blockers.push_back(blocker);
					continue;
				}

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
		if (!LocationJsonUtils::readJsonFile(location_path, data, "LocationJsonLoader"))
			return false;

		LocationDefinition location;
		location.source_path = location_path;
		location.name = data.value("name", LocationJsonUtils::displayNameFromStem(location_path.stem().string()));
		location.objects_path = resolveObjectsPath(location_path, data);

		if (data.contains("map") && data["map"].is_object()) {
			const auto& map_data = data["map"];
			location.map.model = map_data.value("model", location.map.model);
			location.map.scale = map_data.value("scale", location.map.scale);
			location.map.offset = LocationJsonUtils::parseVector3(map_data.value("offset", nlohmann::json::object()));
			location.map.rotation = LocationJsonUtils::parseVector3(map_data.value("rotation", nlohmann::json::object()));
		}

		if (data.contains("player_spawn") && data["player_spawn"].is_object()) {
			location.player_spawn = LocationJsonUtils::parseVector2(data["player_spawn"]);
			location.has_player_spawn = true;
		}

		if (data.contains("navmesh") && data["navmesh"].is_object()) {
			const auto min_height_it = data["navmesh"].find("min_walkable_height");
			if (min_height_it != data["navmesh"].end() && min_height_it->is_number()) {
				location.navmesh_min_walkable_height = min_height_it->get<float>();
				location.has_navmesh_min_walkable_height = true;
			}
		}

		if (data.contains("camera") && data["camera"].is_object()) {
			const auto zoom_it = data["camera"].find("zoom");
			if (zoom_it != data["camera"].end() && zoom_it->is_number()) {
				location.camera_zoom = zoom_it->get<float>();
				location.has_camera_zoom = true;
			}
		} else if (data.contains("camera_zoom") && data["camera_zoom"].is_number()) {
			location.camera_zoom = data["camera_zoom"].get<float>();
			location.has_camera_zoom = true;
		}

		loadEntitiesFromObjectsFile(location);

		out_location = std::move(location);
		return true;
	}

} // namespace Nawia::World
