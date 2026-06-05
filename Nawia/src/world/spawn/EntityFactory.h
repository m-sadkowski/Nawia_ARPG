#pragma once

#include <json.hpp>

#include <memory>
#include <string>

namespace Nawia {
	namespace Core {
		class Engine;
		class Map;
	}

	namespace Entity { class Entity; }
}

namespace Nawia::World {

	/**
	 * @class EntityFactory
	 * @brief Tworzy encje gry na podstawie definicji spawnu z JSON.
	 *
	 * Fabryka mapuje tekstowe pole "type" na konkretne konstruktory C++.
	 * Dzieki temu system spawnow nie musi znac szczegolow tworzenia wrogow,
	 * NPC, skrzyn, teleportow ani dekoracji.
	 */
	class EntityFactory {
	public:
		/**
		 * @brief Tworzy encje z pelnej definicji JSON.
		 *
		 * @param type Typ encji, np. "devil", "chest" albo "npc".
		 * @param data Dane konfiguracyjne encji.
		 * @param engine Silnik potrzebny do dostepu do managerow i zasobow.
		 * @param map Aktualna mapa uzywana m.in. przez wrogow do pathfindingu.
		 * @return Wskaznik na utworzona encje albo nullptr przy bledzie.
		 */
		static std::shared_ptr<Entity::Entity> create(
			const std::string& type,
			const nlohmann::json& data,
			Core::Engine* engine,
			Core::Map* map
		);

	private:
		static std::shared_ptr<Entity::Entity> createDevil(const nlohmann::json& data, Core::Engine* engine, Core::Map* map);
		static std::shared_ptr<Entity::Entity> createWitch(const nlohmann::json& data, Core::Engine* engine, Core::Map* map);
		static std::shared_ptr<Entity::Entity> createBandit(const nlohmann::json& data, Core::Engine* engine, Core::Map* map);
		static std::shared_ptr<Entity::Entity> createWalkingDead(const nlohmann::json& data, Core::Engine* engine, Core::Map* map);
		static std::shared_ptr<Entity::Entity> createFrog(const nlohmann::json& data, Core::Engine* engine, Core::Map* map);
		static std::shared_ptr<Entity::Entity> createWorm(const nlohmann::json& data, Core::Engine* engine, Core::Map* map);
		static std::shared_ptr<Entity::Entity> createMiniMushroomInfected(const nlohmann::json& data, Core::Engine* engine, Core::Map* map);
		static std::shared_ptr<Entity::Entity> createFriend(const nlohmann::json& data, Core::Engine* engine, Core::Map* map);
		static std::shared_ptr<Entity::Entity> createChest(const nlohmann::json& data, Core::Engine* engine);
		static std::shared_ptr<Entity::Entity> createNPC(const nlohmann::json& data, Core::Engine* engine);
		static std::shared_ptr<Entity::Entity> createMiniMushroomProp(const nlohmann::json& data, Core::Engine* engine);
		static std::shared_ptr<Entity::Entity> createStaticObject(const nlohmann::json& data, Core::Engine* engine);
		static std::shared_ptr<Entity::Entity> createCheckpoint(const nlohmann::json& data);
		static std::shared_ptr<Entity::Entity> createMushroomWaypoint(const nlohmann::json& data);
		static std::shared_ptr<Entity::Entity> createStoryAnchor(const nlohmann::json& data);
		static std::shared_ptr<Entity::Entity> createHerbalistHub(const nlohmann::json& data);
		static std::shared_ptr<Entity::Entity> createTeleport(const nlohmann::json& data, Core::Engine* engine);
		static std::shared_ptr<Entity::Entity> createBossTrigger(const nlohmann::json& data, Core::Engine* engine);
		static std::shared_ptr<Entity::Entity> createStoryTrigger(const nlohmann::json& data, Core::Engine* engine);
	};

} // namespace Nawia::World
