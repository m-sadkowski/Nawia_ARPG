#pragma once

#include <NavMesh.h>

#include <raylib.h>

#include <string>
#include <vector>

namespace Nawia::Core {

	class ResourceManager;

	/**
	 * @class Map
	 * @brief Laduje, renderuje i odpytuje model mapy 3D.
	 *
	 * Mapa przechowuje model Raylib oraz navmesh zbudowany z tej samej
	 * geometrii. Ruch encji odbywa sie po plaszczyznie XZ.
	 */
	class Map {
	public:
		explicit Map(ResourceManager& resource_manager);
		~Map();

		/**
		 * @brief Laduje model mapy z `assets/maps`.
		 */
		void loadMap(
			const std::string& filename,
			float scale = 1.0f,
			Vector3 offset = {0.0f, 0.0f, 0.0f},
			Vector3 rotation = {0.0f, 0.0f, 0.0f}
		);

		/**
		 * @brief Laduje awaryjna plaska mape, gdy brakuje modelu.
		 */
		void loadPlaceholder();

		/**
		 * @brief Renderuje mape w aktywnym trybie 3D.
		 */
		void render() const;

		/**
		 * @brief Zwraca trafienie promienia w geometrie mapy.
		 */
		[[nodiscard]] RayCollision getRayCollision(Ray ray) const;

		/**
		 * @brief Sprawdza, czy punkt lezy blisko navmesha.
		 */
		[[nodiscard]] bool isWalkable(float world_x, float world_z) const;

		/**
		 * @brief Zwraca sciezke po navmeshu albo prosta sciezke awaryjna.
		 */
		[[nodiscard]] std::vector<Vector2> findPath(Vector3 start, Vector3 end) const;

		/**
		 * @brief Przebudowuje navmesh z progiem wysokosci dla niewalkowalnej wody.
		 */
		void setNavMeshMinWalkableHeight(float height);

		/**
		 * @brief Zwraca prog wysokosci uzywany przez navmesh.
		 */
		[[nodiscard]] float getNavMeshMinWalkableHeight() const { return _navmesh.getMinWalkableHeight(); }

		[[nodiscard]] Vector2 getPlayerSpawnPos() const { return _player_spawn_pos; }
		Model& getModel() { return _model; }
		const World::NavMesh& getNavMesh() const { return _navmesh; }

	private:
		ResourceManager& _resource_manager;

		Model _model = {};
		bool _model_loaded = false;
		bool _is_placeholder = false;

		float _scale = 1.0f;
		Vector3 _offset = {0.0f, 0.0f, 0.0f};
		Vector3 _rotation = {0.0f, 0.0f, 0.0f};

		Vector2 _player_spawn_pos = {0.0f, 0.0f};

		World::NavMesh _navmesh;
		std::vector<BoundingBox> _mesh_bboxes;
	};

} // namespace Nawia::Core
