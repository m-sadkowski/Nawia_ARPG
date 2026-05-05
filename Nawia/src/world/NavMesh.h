#pragma once

#include <raylib.h>

#include <vector>

class dtNavMesh;
class dtNavMeshQuery;

namespace Nawia::World {

	/**
	 * @class NavMesh
	 * @brief Buduje siatke nawigacji z modelu mapy i wyznacza sciezki.
	 */
	class NavMesh {
	public:
		NavMesh();
		~NavMesh();

		/**
		 * @brief Buduje siatke nawigacji z modelu w przestrzeni swiata.
		 */
		bool buildFromModel(const Model& model, float scale = 1.0f, Vector3 offset = {0.0f, 0.0f, 0.0f});

		/**
		 * @brief Zwraca sciezke po osi XZ miedzy dwoma punktami 3D.
		 */
		std::vector<Vector2> findPath(Vector3 start, Vector3 end) const;

		/**
		 * @brief Zwraca najblizsza poprawna pozycje na siatce nawigacji.
		 */
		Vector3 getClosestWalkablePosition(Vector3 pos) const;

		/**
		 * @brief Sprawdza, czy siatka jest gotowa do zapytan.
		 */
		bool isReady() const { return _navMesh != nullptr && _navQuery != nullptr; }

	private:
		void cleanup();

		dtNavMesh* _navMesh;
		dtNavMeshQuery* _navQuery;
	};

} // namespace Nawia::World
