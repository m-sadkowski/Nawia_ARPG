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
		 * @brief Ustawia minimalna wysokosc powierzchni dopuszczonej do chodzenia.
		 */
		void setMinWalkableHeight(float height) { _min_walkable_height = height; }

		/**
		 * @brief Zwraca minimalna wysokosc powierzchni dopuszczonej do chodzenia.
		 */
		[[nodiscard]] float getMinWalkableHeight() const { return _min_walkable_height; }

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
		float _min_walkable_height = -10000.0f;
	};

} // namespace Nawia::World
