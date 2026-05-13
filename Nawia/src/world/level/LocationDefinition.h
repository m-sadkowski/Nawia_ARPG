#pragma once

#include <json.hpp>
#include <raylib.h>

#include <filesystem>
#include <string>
#include <vector>

namespace Nawia::World {

	struct LocationMapDefinition {
		std::string model = "placeholder";
		float scale = 1.0f;
		Vector3 offset = {0.0f, 0.0f, 0.0f};
		Vector3 rotation = {0.0f, 0.0f, 0.0f};
	};

	struct LocationDefinition {
		std::string name;
		std::filesystem::path source_path;
		std::filesystem::path objects_path;
		LocationMapDefinition map;
		Vector2 player_spawn = {0.0f, 0.0f};
		bool has_player_spawn = false;
		float navmesh_min_walkable_height = 0.0f;
		bool has_navmesh_min_walkable_height = false;
		std::vector<nlohmann::json> entities;
	};

} // namespace Nawia::World
