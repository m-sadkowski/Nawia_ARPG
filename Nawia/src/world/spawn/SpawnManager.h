#pragma once

#include <SpawnPoint.h>

#include <memory>
#include <string>
#include <vector>

namespace Nawia::Core {
	class Engine;
	class Map;
}

namespace Nawia::Item {
	class ItemDatabase;
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
		void update(Vector2 player_pos, const std::string& current_location, Core::Engine* engine);

		/**
		 * @brief Ustawia stany uspienia encji po zmianie lokacji.
		 */
		void updateLocationChange(const std::string& new_location, Core::Engine* engine, Core::Map* map = nullptr);

		/**
		 * @brief Czysci wszystkie spawny.
		 */
		void reset();

		/**
		 * @brief Recznie dodaje spawn do managera.
		 */
		void addSpawnPoint(const SpawnPoint& sp);

		[[nodiscard]] const std::vector<SpawnPoint>& getSpawnPoints() const { return _spawn_points; }

		/**
		 * @brief Zapisuje stan spawnow nalezacych do podanej lokacji.
		 */
		[[nodiscard]] nlohmann::json serializeLocation(const std::string& location_name) const;

		/**
		 * @brief Przywraca stan spawnow lokacji, dopasowujac je po indeksie albo stabilnym ID.
		 */
		void applyLocation(
			const std::string& location_name,
			const nlohmann::json& location_state,
			Item::ItemDatabase& item_database);

	private:
		[[nodiscard]] static std::shared_ptr<Entity::Entity> createEntityForSpawn(
			SpawnPoint& spawn_point,
			Core::Engine* engine,
			Core::Map* map,
			const std::string& current_location,
			bool dormant);
		[[nodiscard]] static std::string makeStableId(const SpawnPoint& spawn_point, size_t index);
		[[nodiscard]] static nlohmann::json serializeSpawn(const SpawnPoint& spawn_point, size_t index);
		void applySpawn(SpawnPoint& spawn_point, const nlohmann::json& state, Item::ItemDatabase& item_database);

		std::vector<SpawnPoint> _spawn_points;
	};

} // namespace Nawia::World
