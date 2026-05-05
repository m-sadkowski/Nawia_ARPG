#pragma once

#include <SpawnPoint.h>

#include <string>
#include <unordered_map>
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
		 * @brief Wczytuje definicje spawnow z JSON i tworzy encje poziomu.
		 *
		 * Encje w aktualnej lokacji bez trigger_radius startuja aktywne, encje
		 * dystansowe oraz encje z innych lokacji pozostaja uspione.
		 *
		 * @param path Sciezka do pliku JSON ze spawnami.
		 * @param engine Silnik potrzebny do fabryki encji i EntityManagera.
		 * @param map Aktualna mapa uzywana m.in. do navmesha.
		 * @param initial_location Nazwa lokacji startowej gracza.
		 * @return true, jesli plik zostal poprawnie obsluzony.
		 */
		bool loadFromJson(
			const std::string& path,
			Core::Engine* engine,
			Core::Map* map,
			const std::string& initial_location
		);

		/**
		 * @brief Sprawdza dystans gracza i aktywuje spawny w biezacej lokacji.
		 */
		void update(Vector2 player_pos, const std::string& current_location);

		/**
		 * @brief Ustawia stany uspienia encji po zmianie lokacji.
		 */
		void updateLocationChange(const std::string& new_location);

		/**
		 * @brief Czysci wszystkie spawny i zapisane pozycje startowe.
		 */
		void reset();

		/**
		 * @brief Pobiera pozycje startowa gracza dla lokacji.
		 */
		bool getPlayerSpawn(const std::string& location_name, Vector2& out_pos) const;

	private:
		std::vector<SpawnPoint> _spawn_points;
		std::unordered_map<std::string, Vector2> _player_spawns;
	};

} // namespace Nawia::World
