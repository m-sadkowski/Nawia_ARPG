#pragma once

#include <json.hpp>
#include <NavMesh.h>
#include <raylib.h>

#include <filesystem>
#include <string>
#include <vector>

namespace Nawia::World {

	/**
	 * @brief Transformacja modelu mapy zapisywana w JSON lokacji.
	 */
	struct LocationMapDefinition {
		std::string model = "placeholder"; ///< ID modelu albo sciezka assetu ladowana przez Map.
		float scale = 1.0f;
		Vector3 offset = {0.0f, 0.0f, 0.0f};   ///< Przesuniecie w swiecie nakladane po skali.
		Vector3 rotation = {0.0f, 0.0f, 0.0f}; ///< Rotacja Eulera zapisana w JSON.
	};

	/**
	 * @brief Robocza reprezentacja jednej lokacji stworzonej w edytorze.
	 *
	 * `source_path` wskazuje glowny JSON lokacji, a `objects_path` to powiazany
	 * JSON obiektow. Flagi `has_*` rozrozniaja wartosci zapisane jawnie od
	 * fallbackow poziomu.
	 */
	struct LocationDefinition {
		std::string name; ///< Nazwa wyswietlana i wewnetrzna lokacji.
		std::filesystem::path source_path;
		std::filesystem::path objects_path;
		LocationMapDefinition map;
		Vector2 player_spawn = {0.0f, 0.0f};
		bool has_player_spawn = false;
		float navmesh_min_walkable_height = 0.0f;
		bool has_navmesh_min_walkable_height = false;
		float camera_zoom = 0.75f;
		bool has_camera_zoom = false;
		std::vector<NavMeshBlocker> navmesh_blockers; ///< Dziury/blokady navmesha zapisane w edytorze.
		std::vector<nlohmann::json> entities;         ///< Surowe definicje spawnow dla SpawnManagera.
	};

} // namespace Nawia::World
