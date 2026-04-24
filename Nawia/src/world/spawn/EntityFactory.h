#pragma once

#include <json.hpp>

#include <memory>
#include <string>

namespace Nawia {
	namespace Core { class Engine; class Map; }
	namespace Entity { class Entity; }
}

namespace Nawia::World {

	/**
	 * @class EntityFactory
	 * @brief Creates game entities from JSON spawn definitions.
	 *
	 * This is the central factory that maps JSON `"type"` strings to concrete
	 * C++ entity constructors. All entity creation for the spawn system goes
	 * through this class.
	 *
	 * Supported types:
	 *  - `"devil"`, `"bandit"`, `"walking_dead"` — enemies (EnemyInterface subclasses)
	 *  - `"chest"` — lootable container
	 *  - `"npc"` — NPC with dialogue/quest support (dispatched via `"npc_class"`)
	 *  - `"static_object"` — decorative/blocking prop
	 *  - `"checkpoint"` — respawn point trigger
	 *
	 * @note The factory needs an Engine pointer to access ResourceManager,
	 *       ItemDatabase, Loottable, DialogueManager, and the current Map.
	 */
	class EntityFactory {
	public:
		/**
		 * @brief Create an entity from a JSON definition.
		 *
		 * @param type   Entity type string (e.g. "devil", "chest", "npc")
		 * @param data   Full JSON object with entity parameters
		 * @param engine Engine pointer for accessing subsystems
		 * @param map    Current level's map (needed by enemies for pathfinding)
		 * @return Shared pointer to the created entity, or nullptr on failure
		 */
		static std::shared_ptr<Entity::Entity> create(
			const std::string& type,
			const nlohmann::json& data,
			Core::Engine* engine,
			Core::Map* map
		);

	private:
		static std::shared_ptr<Entity::Entity> createDevil(const nlohmann::json& data, Core::Engine* engine, Core::Map* map);
		static std::shared_ptr<Entity::Entity> createBandit(const nlohmann::json& data, Core::Engine* engine, Core::Map* map);
		static std::shared_ptr<Entity::Entity> createWalkingDead(const nlohmann::json& data, Core::Engine* engine, Core::Map* map);
		static std::shared_ptr<Entity::Entity> createChest(const nlohmann::json& data, Core::Engine* engine);
		static std::shared_ptr<Entity::Entity> createNPC(const nlohmann::json& data, Core::Engine* engine);
		static std::shared_ptr<Entity::Entity> createStaticObject(const nlohmann::json& data, Core::Engine* engine);
		static std::shared_ptr<Entity::Entity> createCheckpoint(const nlohmann::json& data);
	};

} // namespace Nawia::World
