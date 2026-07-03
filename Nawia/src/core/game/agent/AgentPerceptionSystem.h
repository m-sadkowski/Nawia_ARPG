#pragma once

#include <Ability.h>
#include <CombatEventBus.h>
#include <Entity.h>
#include <MapPingManager.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace Nawia::Core {
	class EntityManager;
}

namespace Nawia::Game {

	enum class AgentRelation {
		Self,
		Ally,
		Enemy,
		Neutral,
		Unknown
	};

	struct AgentEntitySnapshot {
		bool valid = false;
		std::weak_ptr<Entity::Entity> entity;
		Entity::EntityId entity_id = Entity::INVALID_ENTITY_ID;
		std::string name;
		Entity::EntityType type = Entity::EntityType::None;
		Entity::Faction faction = Entity::Faction::None;
		Vector2 position = {0.0f, 0.0f};
		Vector2 velocity = {0.0f, 0.0f};
		int hp = 0;
		int max_hp = 0;
		float hp_ratio = 0.0f;
		bool alive = false;
		bool dying = false;
		bool dormant = false;
		bool visible = false;
		bool interactable = false;
		bool interaction_available = false;
		float interaction_range = 0.0f;
		std::string interaction_state;
		bool moving = false;
		bool rooted = false;
		bool poisoned = false;
		float root_remaining = 0.0f;
		float poison_remaining = 0.0f;
		bool casting = false;
		std::string cast_name;
		float cast_duration = 0.0f;
		float cast_remaining = 0.0f;
		bool cast_interruptible = false;
		bool hazard = false;
		std::string hazard_phase;
		float hazard_radius = 0.0f;
		float hazard_current_radius = 0.0f;
		float hazard_time_to_activate = 0.0f;
		float hazard_remaining = 0.0f;
		int hazard_damage_per_tick = 0;
		float hazard_tick_interval = 0.0f;
		bool hazard_knock_down_player_on_hit = false;
		bool hazard_expanding_wave = false;
		Entity::EntityId hazard_source_entity_id = Entity::INVALID_ENTITY_ID;
	};

	struct AgentObservedEntity {
		AgentEntitySnapshot entity;
		AgentRelation relation = AgentRelation::Unknown;
		float distance = 0.0f;
		Vector2 direction = {0.0f, 0.0f};
		bool is_current_target = false;
	};

	struct AgentLostEntity {
		AgentEntitySnapshot last_known_entity;
		AgentRelation relation = AgentRelation::Unknown;
		Vector2 last_known_position = {0.0f, 0.0f};
		float last_seen_time_seconds = 0.0f;
		float seconds_since_seen = 0.0f;
		bool was_current_target = false;
		std::string disappearance_reason;
	};

	struct AgentAbilitySnapshot {
		int slot = -1;
		std::string name;
		Entity::AbilityTargetType target_type = Entity::AbilityTargetType::POINT;
		bool ready = false;
		bool can_cast = false;
		float cooldown_remaining = 0.0f;
		float cooldown_ratio = 0.0f;
		float cooldown = 0.0f;
		float cast_range = 0.0f;
		float duration = 0.0f;
		float projectile_speed = 0.0f;
		float hitbox_radius = 0.0f;
		int damage = 0;
	};

	struct AgentPerceptionSnapshot {
		std::uint64_t frame_id = 0;
		float time_seconds = 0.0f;
		float perception_radius = 0.0f;
		float event_memory_seconds = 0.0f;
		float lost_memory_seconds = 0.0f;
		float terminal_lost_memory_seconds = 0.0f;
		AgentEntitySnapshot self;
		std::optional<AgentEntitySnapshot> current_target;
		std::optional<AgentEntitySnapshot> last_damage_source;
		std::vector<AgentObservedEntity> observed_entities;
		std::vector<AgentLostEntity> lost_entities;
		std::vector<AgentAbilitySnapshot> abilities;
		std::vector<MapPing> visible_pings;
		std::vector<MapPing> remembered_pings;
		std::vector<CombatEvent> recent_combat_events;
		size_t nearby_enemy_count = 0;
		size_t nearby_ally_count = 0;
		size_t nearby_neutral_count = 0;
		size_t nearby_npc_count = 0;
		size_t nearby_projectile_count = 0;
		size_t nearby_hazard_count = 0;
		size_t lost_entity_count = 0;
	};

	/**
	 * @class AgentPerceptionSystem
	 * @brief Builds read-only world snapshots for future agent decision systems.
	 *
	 * This system does not choose actions. It collects local, factual data:
	 * nearby entities, current target, ability state, and relevant recent combat
	 * events. GOAP, roles, threat, and validation should consume these snapshots
	 * instead of querying the whole world directly.
	 */
	class AgentPerceptionSystem {
	public:
		struct Settings {
			float perception_radius = 12.0f;
			float event_memory_seconds = 3.0f;
			float lost_memory_seconds = 60.0f;
			float terminal_lost_memory_seconds = 4.0f;
			size_t max_observed_entities = 32;
			size_t max_lost_entities = 16;
			size_t max_recent_events = 16;
			bool include_player = true;
			bool include_allies = true;
			bool include_enemies = true;
			bool include_npcs = true;
			bool include_neutral_entities = true;
			bool include_projectiles = true;
		};

		void update(
			const Core::EntityManager& entity_manager,
			const CombatEventBus& combat_event_bus,
			const MapPingManager& ping_manager);
		void clear();

		[[nodiscard]] const Settings& getSettings() const { return _settings; }
		void setSettings(const Settings& settings);

		[[nodiscard]] std::uint64_t getLastFrameId() const { return _last_frame_id; }
		[[nodiscard]] const std::vector<AgentPerceptionSnapshot>& getSnapshots() const { return _snapshots; }
		[[nodiscard]] const AgentPerceptionSnapshot* findSnapshot(Entity::EntityId entity_id) const;
		[[nodiscard]] const AgentPerceptionSnapshot* findSnapshot(const Entity::Entity& entity) const;

	private:
		using EntityLookup = std::unordered_map<Entity::EntityId, std::shared_ptr<Entity::Entity>>;

		struct AgentMemoryRecord {
			AgentEntitySnapshot last_known_entity;
			AgentRelation relation = AgentRelation::Unknown;
			float last_seen_time_seconds = 0.0f;
			std::uint64_t last_seen_frame_id = 0;
			bool was_current_target = false;
			bool terminal = false;
			float terminal_time_seconds = 0.0f;
			std::string terminal_reason;
		};

		[[nodiscard]] bool isAgentCandidate(const std::shared_ptr<Entity::Entity>& entity) const;
		[[nodiscard]] bool isPerceivableEntity(const std::shared_ptr<Entity::Entity>& entity) const;
		[[nodiscard]] AgentPerceptionSnapshot buildSnapshot(
			const std::shared_ptr<Entity::Entity>& agent,
			const std::vector<std::shared_ptr<Entity::Entity>>& entities,
			const EntityLookup& entity_lookup,
			const std::vector<CombatEvent>& recent_events,
			const std::vector<MapPing>& remembered_pings,
			const MapPingManager& ping_manager,
			float time_seconds,
			std::uint64_t frame_id);
		[[nodiscard]] AgentEntitySnapshot makeEntitySnapshot(const std::shared_ptr<Entity::Entity>& entity) const;
		[[nodiscard]] AgentAbilitySnapshot makeAbilitySnapshot(
			const std::shared_ptr<Entity::Ability>& ability,
			int slot) const;
		[[nodiscard]] AgentRelation getRelation(
			const Entity::Entity& observer,
			const Entity::Entity& observed) const;
		[[nodiscard]] bool isEventRelevantToAgent(
			const CombatEvent& event,
			Entity::EntityId agent_id,
			Vector2 agent_position) const;
		[[nodiscard]] bool isPingRelevantToAgent(const Entity::Entity& agent, const MapPing& ping) const;
		void updateMemory(
			AgentPerceptionSnapshot& snapshot,
			const std::vector<AgentObservedEntity>& all_observed_entities,
			const std::shared_ptr<Entity::Entity>& agent,
			const EntityLookup& entity_lookup,
			float time_seconds,
			std::uint64_t frame_id);
		[[nodiscard]] std::shared_ptr<Entity::Entity> findEntityById(
			const EntityLookup& entity_lookup,
			Entity::EntityId entity_id) const;
		[[nodiscard]] std::string getDisappearanceReason(
			const std::shared_ptr<Entity::Entity>& entity,
			Vector2 agent_position) const;

		Settings _settings;
		std::uint64_t _next_frame_id = 1;
		std::uint64_t _last_frame_id = 0;
		std::vector<AgentPerceptionSnapshot> _snapshots;
		std::map<Entity::EntityId, std::map<Entity::EntityId, AgentMemoryRecord>> _memory_by_agent;
	};

	[[nodiscard]] const char* toString(AgentRelation relation);

} // namespace Nawia::Game
