#include "AgentCommandInterface.h"

#include <Engine.h>
#include <Entity.h>
#include <EntityManager.h>

#include <algorithm>

namespace Nawia::Game {

	void AgentCommandInterface::update(
		Core::Engine& engine,
		Core::EntityManager& entity_manager,
		const float dt)
	{
		_time_seconds += std::max(0.0f, dt);

		for (auto it = _active_commands.begin(); it != _active_commands.end();) {
			auto& command = it->second;
			command.age_seconds += std::max(0.0f, dt);
			updateCommand(command, engine, entity_manager, dt);

			if (isTerminal(command.status)) {
				rememberCompleted(command);
				it = _active_commands.erase(it);
			} else {
				++it;
			}
		}
	}

	void AgentCommandInterface::clear() {
		_active_commands.clear();
		_completed_commands.clear();
		_next_command_id = 1;
		_time_seconds = 0.0f;
	}

	AgentCommandState AgentCommandInterface::submit(const AgentCommandRequest& request) {
		AgentCommandState command;
		command.command_id = _next_command_id++;
		command.request = request;
		command.request.acceptance_radius = std::max(0.01f, command.request.acceptance_radius);
		command.status = AgentCommandStatus::Queued;
		command.started_time_seconds = _time_seconds;

		if (request.agent_entity_id == Entity::INVALID_ENTITY_ID) {
			complete(command, AgentCommandStatus::Failed, AgentCommandFailureReason::NoAgent, "Missing agent entity id.");
			rememberCompleted(command);
			return command;
		}

		if (auto existing = _active_commands.find(request.agent_entity_id); existing != _active_commands.end()) {
			complete(existing->second, AgentCommandStatus::Cancelled, AgentCommandFailureReason::CommandReplaced, "Command replaced.");
			rememberCompleted(existing->second);
			_active_commands.erase(existing);
		}

		_active_commands[request.agent_entity_id] = command;
		return command;
	}

	AgentCommandState AgentCommandInterface::submitMoveTo(
		const Entity::EntityId agent_entity_id,
		const Vector3 position,
		const float acceptance_radius)
	{
		AgentCommandRequest request;
		request.type = AgentCommandType::MoveTo;
		request.agent_entity_id = agent_entity_id;
		request.has_world_position = true;
		request.world_position = position;
		request.acceptance_radius = acceptance_radius > 0.0f ? acceptance_radius : _settings.default_move_acceptance_radius;
		return submit(request);
	}

	AgentCommandState AgentCommandInterface::submitMoveToEntity(
		const Entity::EntityId agent_entity_id,
		const Entity::EntityId target_entity_id,
		const float desired_range)
	{
		AgentCommandRequest request;
		request.type = AgentCommandType::MoveToEntity;
		request.agent_entity_id = agent_entity_id;
		request.target_entity_id = target_entity_id;
		request.acceptance_radius = desired_range > 0.0f ? desired_range : _settings.default_move_acceptance_radius;
		return submit(request);
	}

	AgentCommandState AgentCommandInterface::submitStop(const Entity::EntityId agent_entity_id) {
		AgentCommandRequest request;
		request.type = AgentCommandType::Stop;
		request.agent_entity_id = agent_entity_id;
		return submit(request);
	}

	AgentCommandState AgentCommandInterface::submitAttack(
		const Entity::EntityId agent_entity_id,
		const Entity::EntityId target_entity_id)
	{
		AgentCommandRequest request;
		request.type = AgentCommandType::Attack;
		request.agent_entity_id = agent_entity_id;
		request.target_entity_id = target_entity_id;
		request.ability_slot = 0;
		request.acceptance_radius = _settings.default_move_acceptance_radius;
		return submit(request);
	}

	AgentCommandState AgentCommandInterface::submitCastAbilityAtTarget(
		const Entity::EntityId agent_entity_id,
		const int ability_slot,
		const Entity::EntityId target_entity_id)
	{
		AgentCommandRequest request;
		request.type = AgentCommandType::CastAbility;
		request.agent_entity_id = agent_entity_id;
		request.target_entity_id = target_entity_id;
		request.ability_slot = ability_slot;
		return submit(request);
	}

	AgentCommandState AgentCommandInterface::submitCastAbilityAtPosition(
		const Entity::EntityId agent_entity_id,
		const int ability_slot,
		const Vector3 position)
	{
		AgentCommandRequest request;
		request.type = AgentCommandType::CastAbility;
		request.agent_entity_id = agent_entity_id;
		request.ability_slot = ability_slot;
		request.has_world_position = true;
		request.world_position = position;
		return submit(request);
	}

	AgentCommandState AgentCommandInterface::submitInteract(
		const Entity::EntityId agent_entity_id,
		const Entity::EntityId target_entity_id)
	{
		AgentCommandRequest request;
		request.type = AgentCommandType::Interact;
		request.agent_entity_id = agent_entity_id;
		request.target_entity_id = target_entity_id;
		request.acceptance_radius = _settings.default_move_acceptance_radius;
		return submit(request);
	}

	bool AgentCommandInterface::cancel(
		const Entity::EntityId agent_entity_id,
		const AgentCommandFailureReason reason)
	{
		const auto it = _active_commands.find(agent_entity_id);
		if (it == _active_commands.end())
			return false;

		complete(it->second, AgentCommandStatus::Cancelled, reason, "Command cancelled.");
		rememberCompleted(it->second);
		_active_commands.erase(it);
		return true;
	}

	void AgentCommandInterface::cancelAll(const AgentCommandFailureReason reason) {
		for (auto& [_, command] : _active_commands) {
			complete(command, AgentCommandStatus::Cancelled, reason, "Command cancelled.");
			rememberCompleted(command);
		}
		_active_commands.clear();
	}

	void AgentCommandInterface::setSettings(const Settings& settings) {
		_settings = settings;
		_settings.default_move_acceptance_radius = std::max(0.01f, _settings.default_move_acceptance_radius);
		_settings.attack_path_rebuild_interval = std::max(0.05f, _settings.attack_path_rebuild_interval);
		_settings.interact_path_rebuild_interval = std::max(0.05f, _settings.interact_path_rebuild_interval);
		_settings.max_completed_history = std::max<size_t>(1, _settings.max_completed_history);
	}

	std::optional<AgentCommandState> AgentCommandInterface::getCommandForAgent(const Entity::EntityId agent_entity_id) const {
		const auto it = _active_commands.find(agent_entity_id);
		if (it == _active_commands.end())
			return std::nullopt;
		return it->second;
	}

	void AgentCommandInterface::updateCommand(
		AgentCommandState& command,
		Core::Engine& engine,
		Core::EntityManager& entity_manager,
		const float dt)
	{
		if (command.status == AgentCommandStatus::Queued)
			command.status = AgentCommandStatus::Running;

		const auto agent = findEntityById(entity_manager, command.request.agent_entity_id);
		if (!isAgentAvailable(agent)) {
			complete(command, AgentCommandStatus::Failed, AgentCommandFailureReason::AgentUnavailable, "Agent is not available.");
			return;
		}

		if (command.request.type != AgentCommandType::Stop && isControlLocked(*agent)) {
			complete(command, AgentCommandStatus::Failed, AgentCommandFailureReason::ControlLocked, "Agent control is locked.");
			return;
		}

		switch (command.request.type) {
			case AgentCommandType::MoveTo:
				updateMoveTo(command, engine, agent);
				break;
			case AgentCommandType::MoveToEntity: {
				const auto target = findEntityById(entity_manager, command.request.target_entity_id);
				updateMoveToEntity(command, engine, agent, target, dt);
				break;
			}
			case AgentCommandType::Stop:
				stopEntity(agent);
				complete(command, AgentCommandStatus::Succeeded, AgentCommandFailureReason::None, "Agent stopped.");
				break;
			case AgentCommandType::Attack: {
				const auto target = findEntityById(entity_manager, command.request.target_entity_id);
				updateAttack(command, engine, agent, target, dt);
				break;
			}
			case AgentCommandType::CastAbility: {
				const auto target = findEntityById(entity_manager, command.request.target_entity_id);
				updateCastAbility(command, engine, agent, target);
				break;
			}
			case AgentCommandType::Interact: {
				const auto target = findEntityById(entity_manager, command.request.target_entity_id);
				updateInteract(command, engine, agent, target, dt);
				break;
			}
			case AgentCommandType::None:
			default:
				complete(command, AgentCommandStatus::Failed, AgentCommandFailureReason::Cancelled, "Empty command.");
				break;
		}
	}

} // namespace Nawia::Game
