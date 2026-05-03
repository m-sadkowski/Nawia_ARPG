#pragma once
#include "ResourceManager.h"

#include <NavMesh.h>

#include <raylib.h>
#include <string>
#include <vector>

namespace Nawia::Core {

	/**
	 * @class Map
	 * @brief Loads and renders a 3D map model (.glb) or a procedural placeholder.
	 *
	 * The map is a 3D model placed at origin. Entity movement happens on the XZ plane (Y=0).
	 * Walkability and pathfinding will be added in a later phase.
	 */
	class Map {
	public:
		explicit Map(ResourceManager& resource_manager);
		~Map();

		/**
		 * @brief Load a 3D map model from a .glb file.
		 * @param filename Path relative to assets/maps/ (e.g. "demo_map.glb")
		 * @param scale Optional scaling factor
		 * @param offset Optional position offset for the model
		 * @param rotation Optional rotation in degrees (Euler angles: X, Y, Z)
		 */
		void loadMap(const std::string& filename, float scale = 1.0f, Vector3 offset = {0.0f, 0.0f, 0.0f}, Vector3 rotation = {0.0f, 0.0f, 0.0f});

		/**
		 * @brief Generate a placeholder ground plane (colored grid).
		 * Use this when no .glb map is available.
		 */
		void loadPlaceholder();

		/**
		 * @brief Render the map in the current 3D mode context.
		 * Must be called between BeginMode3D/EndMode3D.
		 */
		void render() const;
		
		/** @brief Raycast against the map geometry. */
		[[nodiscard]] RayCollision getRayCollision(Ray ray) const;

		/// @brief Always returns true (walkability disabled for now)
		[[nodiscard]] bool isWalkable(float world_x, float world_z) const;

		/// @brief Returns empty path (pathfinding disabled for now)
		[[nodiscard]] std::vector<Vector2> findPath(Vector3 start, Vector3 end) const;

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