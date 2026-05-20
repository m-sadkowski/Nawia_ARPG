#pragma once

#include <SpawnPoint.h>

#include <string>
#include <vector>

namespace Nawia::Core {
	class Engine;
	class Map;
}

namespace Nawia::World {

	/**
	 * @class SpawnManager
	 * @brief Wczytuje spawny poziomu i aktywuje encje w odpowiednim momencie.
	 *
	 * Encje sa tworzone podczas ladowania poziomu i dodawane do EntityManagera
	 * w stanie aktywnym albo uspionym. Dzieki temu wejscie w zasieg spawnu nie
	 * laduje modeli w trakcie rozgrywki, tylko przelacza flage dormant.
	 */
	class SpawnManager {
	public:
		SpawnManager() = default;

		/**
		 * @brief Tworzy encje poziomu z definicji wczytanych przez loader lokacji.
		 *
		 * Encje w aktualnej lokacji bez trigger_radius startuja aktywne, encje
		 * dystansowe oraz encje z innych lokacji pozostaja uspione.
		 *
		 * @param entities Lista encji z polem "location" dodanym przez poziom.
		 * @param engine Silnik potrzebny do fabryki encji i EntityManagera.
		 * @param map Aktualna mapa uzywana m.in. do navmesha.
		 * @param current_location Nazwa lokacji aktywnej przy tworzeniu puli.
		 * @return true, jesli definicje zostaly poprawnie obsluzone.
		 */
		bool loadEntities(
			const std::vector<nlohmann::json>& entities,
			Core::Engine* engine,
			Core::Map* map,
			const std::string& current_location,
			const std::string& source_label = "runtime"
		);

		/**
		 * @brief Sprawdza dystans gracza i aktywuje spawny w biezacej lokacji.
		 */
		void update(Vector2 player_pos, const std::string& current_location);

		/**
		 * @brief Ustawia stany uspienia encji po zmianie lokacji.
		 */
		void updateLocationChange(const std::string& new_location, Core::Map* map = nullptr);

		/**
		 * @brief Czysci wszystkie spawny.
		 */
		void reset();

		/**
		 * @brief Recznie dodaje spawn do managera.
		 */
		void addSpawnPoint(const SpawnPoint& sp);

		[[nodiscard]] const std::vector<SpawnPoint>& getSpawnPoints() const { return _spawn_points; }
		std::vector<SpawnPoint>& getSpawnPoints() { return _spawn_points; }

	private:
		std::vector<SpawnPoint> _spawn_points;
	};

} // namespace Nawia::World
