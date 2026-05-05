#pragma once

#include <json.hpp>
#include <raylib.h>

#include <memory>
#include <string>

namespace Nawia::Entity { class Entity; }

namespace Nawia::World {

	/**
	 * @struct SpawnPoint
	 * @brief Opis pojedynczego spawnu wczytanego z JSON.
	 *
	 * Spawn nalezy do konkretnej lokacji poziomu i przechowuje dane potrzebne
	 * do stworzenia encji, ustawienia jej pozycji oraz aktywowania jej dopiero
	 * wtedy, gdy gracz znajdzie sie wystarczajaco blisko.
	 */
	struct SpawnPoint {
		std::string location;       ///< Nazwa lokacji, do ktorej nalezy spawn.
		std::string entity_type;    ///< Klucz fabryki, np. "devil", "chest" albo "npc".
		nlohmann::json entity_data; ///< Pelny JSON przekazywany do EntityFactory.

		std::shared_ptr<Entity::Entity> entity; ///< Encja tworzona przy ladowaniu poziomu.

		Vector2 spawn_center = {0, 0}; ///< Srodek spawnu i punkt testu zasiegu.
		float trigger_radius = 0.0f;   ///< 0 aktywuje od razu, wartosc dodatnia wymaga podejscia.
		float spawn_radius = 0.0f;     ///< Promien losowego przesuniecia od srodka spawnu.

		bool activated = false;        ///< Czy encja zostala juz obudzona.
		bool respawnable = false;      ///< Czy encja moze odrodzic sie po smierci.
		float respawn_cooldown = 0.0f; ///< Czas odrodzenia w sekundach.
		float respawn_timer = 0.0f;    ///< Aktualny licznik odrodzenia.

		/**
		 * @brief Zeruje stan runtime spawnu przed ponownym wczytaniem poziomu.
		 */
		void reset() {
			activated = false;
			respawn_timer = 0.0f;
			entity.reset();
		}
	};

} // namespace Nawia::World
