#pragma once
#include <raylib.h>
#include <vector>

class dtNavMesh;
class dtNavMeshQuery;

namespace Nawia::World {

	class NavMesh {
	public:
		NavMesh();
		~NavMesh();

		// Builds the navigation mesh from a loaded Raylib model.
		// Assumes the model is placed at origin. Scale is applied to vertices.
		bool buildFromModel(const Model& model, float scale = 1.0f);

		// Finds a path between two 3D world points. Returns 2D XZ path.
		std::vector<Vector2> findPath(Vector3 start, Vector3 end) const;

		// Gets the closest valid walkable position on the navmesh (useful for snapping entities)
		Vector3 getClosestWalkablePosition(Vector3 pos) const;

		bool isReady() const { return _navMesh != nullptr && _navQuery != nullptr; }

	private:
		void cleanup();

		dtNavMesh* _navMesh;
		dtNavMeshQuery* _navQuery;
	};

} // namespace Nawia::World
