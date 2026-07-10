#pragma once

#include <Entity.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <raylib.h>
#include <string>
#include <vector>

namespace Nawia::Core {
	class Engine;
	class EntityManager;
}

namespace Nawia::Game {

	enum class AgentCommandType {
		None,
		MoveTo,
		MoveToEntity,
		Stop,
		Attack,
		CastAbility,
		Interact
	};

	enum class AgentCommandStatus {
		Queued,
		Running,
		Succeeded,
		Failed,
		Cancelled
	};

	enum class AgentCommandFailureReason {
		None,
		NoAgent,
		AgentUnavailable,
		NoTarget,
		TargetUnavailable,
		TargetOutOfRange,
		NoAbility,
		AbilityOnCooldown,
		AbilityTargetMismatch,
		ControlLocked,
		MovementRooted,
		InteractionUnavailable,
		PathUnavailable,
		CommandReplaced,
		Cancelled
	};

	struct AgentCommandRequest {
		AgentCommandType type = AgentCommandType::None;
		Entity::EntityId agent_entity_id = Entity::INVALID_ENTITY_ID;
		Entity::EntityId target_entity_id = Entity::INVALID_ENTITY_ID;
		int ability_slot = -1;
		bool has_world_position = false;
		Vector3 world_position = {0.0f, 0.0f, 0.0f};
		float acceptance_radius = 0.25f;
	};

	struct AgentCommandState {
		std::uint64_t command_id = 0;
		AgentCommandRequest request;
		AgentCommandStatus status = AgentCommandStatus::Queued;
		AgentCommandFailureReason failure_reason = AgentCommandFailureReason::None;
		float age_seconds = 0.0f;
		float started_time_seconds = 0.0f;
		float completed_time_seconds = 0.0f;
		std::string message;

		// Runtime-only execution data. Kept here so callers can inspect path state
		// during debugging, but decision systems should treat it as read-only.
		std::vector<Vector2> path;
		size_t path_index = 0;
		float path_rebuild_timer = 0.0f;
	};

	/**
	 * @class AgentCommandInterface
	 * @brief Executes explicit commands for entities without choosing actions.
	 *
	 * This is an execution adapter for external tooling and tests. It validates
	 * runtime preconditions, resolves entity ids, starts movement, abilities, and
	 * interactions, then exposes command status.
	 */
	class AgentCommandInterface {
	public:
		struct Settings {
			float default_move_acceptance_radius = 0.25f;
			float attack_path_rebuild_interval = 0.35f;
			float attack_preferred_range_fraction = 2.0f / 3.0f;
			float interact_path_rebuild_interval = 0.35f;
			size_t max_completed_history = 64;
		};

		void update(Core::Engine& engine, Core::EntityManager& entity_manager, float dt);
		void clear();

		[[nodiscard]] AgentCommandState submit(const AgentCommandRequest& request);
		[[nodiscard]] AgentCommandState submitMoveTo(Entity::EntityId agent_entity_id, Vector3 position, float acceptance_radius = 0.25f);
		[[nodiscard]] AgentCommandState submitMoveToEntity(
			Entity::EntityId agent_entity_id,
			Entity::EntityId target_entity_id,
			float desired_range = 0.75f);
		[[nodiscard]] AgentCommandState submitStop(Entity::EntityId agent_entity_id);
		[[nodiscard]] AgentCommandState submitAttack(Entity::EntityId agent_entity_id, Entity::EntityId target_entity_id);
		[[nodiscard]] AgentCommandState submitCastAbilityAtTarget(
			Entity::EntityId agent_entity_id,
			int ability_slot,
			Entity::EntityId target_entity_id);
		[[nodiscard]] AgentCommandState submitCastAbilityAtPosition(
			Entity::EntityId agent_entity_id,
			int ability_slot,
			Vector3 position);
		[[nodiscard]] AgentCommandState submitInteract(Entity::EntityId agent_entity_id, Entity::EntityId target_entity_id);

		bool cancel(Entity::EntityId agent_entity_id, AgentCommandFailureReason reason = AgentCommandFailureReason::Cancelled);
		void cancelAll(AgentCommandFailureReason reason = AgentCommandFailureReason::Cancelled);

		[[nodiscard]] const Settings& getSettings() const { return _settings; }
		void setSettings(const Settings& settings);

		[[nodiscard]] const std::map<Entity::EntityId, AgentCommandState>& getActiveCommands() const { return _active_commands; }
		[[nodiscard]] const std::vector<AgentCommandState>& getCompletedCommands() const { return _completed_commands; }
		[[nodiscard]] std::optional<AgentCommandState> getCommandForAgent(Entity::EntityId agent_entity_id) const;

	private:
		void updateCommand(
			AgentCommandState& command,
			Core::Engine& engine,
			Core::EntityManager& entity_manager,
			float dt);
		void updateMoveTo(AgentCommandState& command, Core::Engine& engine, const std::shared_ptr<Entity::Entity>& agent);
		void updateMoveToEntity(
			AgentCommandState& command,
			Core::Engine& engine,
			const std::shared_ptr<Entity::Entity>& agent,
			const std::shared_ptr<Entity::Entity>& target,
			float dt);
		void updateAttack(
			AgentCommandState& command,
			Core::Engine& engine,
			const std::shared_ptr<Entity::Entity>& agent,
			const std::shared_ptr<Entity::Entity>& target,
			float dt);
		void updateCastAbility(
			AgentCommandState& command,
			Core::Engine& engine,
			const std::shared_ptr<Entity::Entity>& agent,
			const std::shared_ptr<Entity::Entity>& target);
		void updateInteract(
			AgentCommandState& command,
			Core::Engine& engine,
			const std::shared_ptr<Entity::Entity>& agent,
			const std::shared_ptr<Entity::Entity>& target,
			float dt);

		[[nodiscard]] std::shared_ptr<Entity::Entity> findEntityById(
			const Core::EntityManager& entity_manager,
			Entity::EntityId entity_id) const;
		[[nodiscard]] bool isAgentAvailable(const std::shared_ptr<Entity::Entity>& agent) const;
		[[nodiscard]] bool isTargetAvailable(const std::shared_ptr<Entity::Entity>& target) const;
		[[nodiscard]] bool isControlLocked(const Entity::Entity& entity) const;
		[[nodiscard]] bool rebuildPath(
			AgentCommandState& command,
			Core::Engine& engine,
			const std::shared_ptr<Entity::Entity>& agent,
			Vector3 destination) const;
		[[nodiscard]] bool updatePathMovement(AgentCommandState& command, const std::shared_ptr<Entity::Entity>& agent) const;
		void stopEntity(const std::shared_ptr<Entity::Entity>& entity) const;
		void complete(AgentCommandState& command, AgentCommandStatus status, AgentCommandFailureReason reason, std::string message = {});
		void rememberCompleted(AgentCommandState command);
		[[nodiscard]] static bool isTerminal(AgentCommandStatus status);

		Settings _settings;
		std::uint64_t _next_command_id = 1;
		float _time_seconds = 0.0f;
		std::map<Entity::EntityId, AgentCommandState> _active_commands;
		std::vector<AgentCommandState> _completed_commands;
	};

	[[nodiscard]] const char* toString(AgentCommandType type);
	[[nodiscard]] const char* toString(AgentCommandStatus status);
	[[nodiscard]] const char* toString(AgentCommandFailureReason reason);

} // namespace Nawia::Game
