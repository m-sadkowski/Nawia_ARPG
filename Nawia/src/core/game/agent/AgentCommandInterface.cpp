#include "AgentCommandInterface.h"

#include <Ability.h>
#include <Engine.h>
#include <Entity.h>
#include <EntityManager.h>
#include <Interactable.h>
#include <Map.h>
#include <Player.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace Nawia::Game {

	namespace {
		[[nodiscard]] float distanceSquared(const Vector2 first, const Vector2 second) {
			const float dx = second.x - first.x;
			const float dy = second.y - first.y;
			return dx * dx + dy * dy;
		}

		[[nodiscard]] float distanceToBoxSquared(const Entity::Entity& entity, const Vector2 position) {
			const BoundingBox box = entity.getBoundingBox();
			const float closest_x = std::clamp(position.x, box.min.x, box.max.x);
			const float closest_y = std::clamp(position.y, box.min.z, box.max.z);
			const float dx = position.x - closest_x;
			const float dy = position.y - closest_y;
			return dx * dx + dy * dy;
		}

	}

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

	void AgentCommandInterface::updateMoveToEntity(
		AgentCommandState& command,
		Core::Engine& engine,
		const std::shared_ptr<Entity::Entity>& agent,
		const std::shared_ptr<Entity::Entity>& target,
		const float dt)
	{
		if (!isTargetAvailable(target)) {
			complete(command, AgentCommandStatus::Failed, AgentCommandFailureReason::NoTarget, "Move target is not available.");
			return;
		}
		if (agent->isMovementRooted()) {
			complete(command, AgentCommandStatus::Failed, AgentCommandFailureReason::MovementRooted, "Agent is movement-rooted.");
			return;
		}

		if (distanceToBoxSquared(*target, agent->getCenter()) <= command.request.acceptance_radius * command.request.acceptance_radius) {
			stopEntity(agent);
			complete(command, AgentCommandStatus::Succeeded, AgentCommandFailureReason::None, "Target range reached.");
			return;
		}

		command.path_rebuild_timer -= dt;
		if (command.path.empty() || command.path_rebuild_timer <= 0.0f) {
			if (!rebuildPath(command, engine, agent, target->getWorldPos3D())) {
				complete(command, AgentCommandStatus::Failed, AgentCommandFailureReason::PathUnavailable, "No path to move target.");
				return;
			}
			command.path_rebuild_timer = _settings.interact_path_rebuild_interval;
		}

		(void)updatePathMovement(command, agent);
	}

	void AgentCommandInterface::updateMoveTo(
		AgentCommandState& command,
		Core::Engine& engine,
		const std::shared_ptr<Entity::Entity>& agent)
	{
		if (!command.request.has_world_position) {
			complete(command, AgentCommandStatus::Failed, AgentCommandFailureReason::PathUnavailable, "Move command has no destination.");
			return;
		}
		if (agent->isMovementRooted()) {
			complete(command, AgentCommandStatus::Failed, AgentCommandFailureReason::MovementRooted, "Agent is movement-rooted.");
			return;
		}

		const Vector2 destination = {command.request.world_position.x, command.request.world_position.z};
		if (distanceSquared(agent->getCenter(), destination) <= command.request.acceptance_radius * command.request.acceptance_radius) {
			stopEntity(agent);
			complete(command, AgentCommandStatus::Succeeded, AgentCommandFailureReason::None, "Destination reached.");
			return;
		}

		if (command.path.empty() && !rebuildPath(command, engine, agent, command.request.world_position)) {
			complete(command, AgentCommandStatus::Failed, AgentCommandFailureReason::PathUnavailable, "No path to destination.");
			return;
		}

		if (updatePathMovement(command, agent))
			return;

		if (distanceSquared(agent->getCenter(), destination) <= command.request.acceptance_radius * command.request.acceptance_radius) {
			stopEntity(agent);
			complete(command, AgentCommandStatus::Succeeded, AgentCommandFailureReason::None, "Destination reached.");
		}
	}

	void AgentCommandInterface::updateAttack(
		AgentCommandState& command,
		Core::Engine& engine,
		const std::shared_ptr<Entity::Entity>& agent,
		const std::shared_ptr<Entity::Entity>& target,
		const float dt)
	{
		if (!target) {
			complete(command, AgentCommandStatus::Failed, AgentCommandFailureReason::NoTarget, "Attack target is missing.");
			return;
		}
		if (target->isDead() || target->isDying()) {
			complete(command, AgentCommandStatus::Succeeded, AgentCommandFailureReason::None, "Target is no longer active.");
			return;
		}
		if (target->isDormant()) {
			complete(command, AgentCommandStatus::Failed, AgentCommandFailureReason::TargetUnavailable, "Attack target is dormant.");
			return;
		}

		const auto ability = agent->getAbility(command.request.ability_slot);
		if (!ability) {
			complete(command, AgentCommandStatus::Failed, AgentCommandFailureReason::NoAbility, "Primary attack ability is missing.");
			return;
		}

		agent->setTarget(target);

		if (agent->isAnimationLocked())
			return;

		const float distance = std::sqrt(distanceSquared(agent->getCenter(), target->getCenter()));
		const float cast_range = std::max(0.0f, ability->getCastRange());
		const float preferred_range_fraction = std::clamp(_settings.attack_preferred_range_fraction, 0.05f, 1.0f);
		const float preferred_attack_range = cast_range * preferred_range_fraction;
		if (distance <= preferred_attack_range) {
			stopEntity(agent);
			agent->rotateTowardsCenter(target->getCenter().x, target->getCenter().y);
			if (ability->isReady()) {
				if (auto effect = ability->cast(target->getCenter().x, target->getCenter().y))
					engine.spawnEntity(effect);
			}
			return;
		}

		if (agent->isMovementRooted())
			return;

		command.path_rebuild_timer -= dt;
		if (command.path.empty() || command.path_rebuild_timer <= 0.0f) {
			const Vector3 destination = target->getWorldPos3D();
			if (!rebuildPath(command, engine, agent, destination))
				return;
			command.path_rebuild_timer = _settings.attack_path_rebuild_interval;
		}

		(void)updatePathMovement(command, agent);
	}

	void AgentCommandInterface::updateCastAbility(
		AgentCommandState& command,
		Core::Engine& engine,
		const std::shared_ptr<Entity::Entity>& agent,
		const std::shared_ptr<Entity::Entity>& target)
	{
		const auto ability = agent->getAbility(command.request.ability_slot);
		if (!ability) {
			complete(command, AgentCommandStatus::Failed, AgentCommandFailureReason::NoAbility, "Ability slot is empty.");
			return;
		}
		if (!ability->isReady()) {
			complete(command, AgentCommandStatus::Failed, AgentCommandFailureReason::AbilityOnCooldown, "Ability is on cooldown.");
			return;
		}
		if (agent->isAnimationLocked())
			return;

		Vector2 target_position = agent->getCenter();
		switch (ability->getTargetType()) {
			case Entity::AbilityTargetType::UNIT:
				if (!isTargetAvailable(target)) {
					complete(command, AgentCommandStatus::Failed, AgentCommandFailureReason::NoTarget, "Unit-targeted ability has no target.");
					return;
				}
				target_position = target->getCenter();
				agent->setTarget(target);
				break;
			case Entity::AbilityTargetType::POINT:
				if (!command.request.has_world_position) {
					complete(command, AgentCommandStatus::Failed, AgentCommandFailureReason::AbilityTargetMismatch, "Point-targeted ability has no position.");
					return;
				}
				target_position = {command.request.world_position.x, command.request.world_position.z};
				break;
			case Entity::AbilityTargetType::SELF:
				target_position = agent->getCenter();
				break;
		}

		const float cast_range = ability->getCastRange();
		if (cast_range > 0.0f && distanceSquared(agent->getCenter(), target_position) > cast_range * cast_range) {
			complete(command, AgentCommandStatus::Failed, AgentCommandFailureReason::TargetOutOfRange, "Ability target is out of range.");
			return;
		}

		stopEntity(agent);
		agent->rotateTowardsCenter(target_position.x, target_position.y);
		if (auto effect = ability->cast(target_position.x, target_position.y))
			engine.spawnEntity(effect);

		complete(command, AgentCommandStatus::Succeeded, AgentCommandFailureReason::None, "Ability cast started.");
	}

	void AgentCommandInterface::updateInteract(
		AgentCommandState& command,
		Core::Engine& engine,
		const std::shared_ptr<Entity::Entity>& agent,
		const std::shared_ptr<Entity::Entity>& target,
		const float dt)
	{
		if (!isTargetAvailable(target)) {
			complete(command, AgentCommandStatus::Failed, AgentCommandFailureReason::NoTarget, "Interaction target is not available.");
			return;
		}

		auto* interactable = dynamic_cast<Entity::Interactable*>(target.get());
		if (!interactable || !interactable->canInteract()) {
			complete(command, AgentCommandStatus::Failed, AgentCommandFailureReason::InteractionUnavailable, "Interaction is unavailable.");
			return;
		}

		const float interaction_range = std::max(0.0f, interactable->getInteractionRange());
		const float interaction_range_sq = interaction_range * interaction_range;
		if (distanceToBoxSquared(*target, agent->getCenter()) <= interaction_range_sq) {
			stopEntity(agent);
			interactable->onInteract(*agent);
			interactable->onInteractionCompleted(*agent, engine);
			complete(command, AgentCommandStatus::Succeeded, AgentCommandFailureReason::None, "Interaction completed.");
			return;
		}

		if (agent->isMovementRooted()) {
			complete(command, AgentCommandStatus::Failed, AgentCommandFailureReason::MovementRooted, "Agent is movement-rooted.");
			return;
		}

		command.path_rebuild_timer -= dt;
		if (command.path.empty() || command.path_rebuild_timer <= 0.0f) {
			if (!rebuildPath(command, engine, agent, target->getWorldPos3D())) {
				complete(command, AgentCommandStatus::Failed, AgentCommandFailureReason::PathUnavailable, "No path to interaction target.");
				return;
			}
			command.path_rebuild_timer = _settings.interact_path_rebuild_interval;
		}

		(void)updatePathMovement(command, agent);
	}

	std::shared_ptr<Entity::Entity> AgentCommandInterface::findEntityById(
		const Core::EntityManager& entity_manager,
		const Entity::EntityId entity_id) const
	{
		if (entity_id == Entity::INVALID_ENTITY_ID)
			return nullptr;

		for (const auto& entity : entity_manager.getEntities()) {
			if (entity && entity->getEntityId() == entity_id)
				return entity;
		}
		return nullptr;
	}

	bool AgentCommandInterface::isAgentAvailable(const std::shared_ptr<Entity::Entity>& agent) const {
		return agent && !agent->isDead() && !agent->isDying() && !agent->isDormant();
	}

	bool AgentCommandInterface::isTargetAvailable(const std::shared_ptr<Entity::Entity>& target) const {
		return target && !target->isDead() && !target->isDying() && !target->isDormant();
	}

	bool AgentCommandInterface::isControlLocked(const Entity::Entity& entity) const {
		const auto* player = dynamic_cast<const Entity::Player*>(&entity);
		return player && player->isControlLocked();
	}

	bool AgentCommandInterface::rebuildPath(
		AgentCommandState& command,
		Core::Engine& engine,
		const std::shared_ptr<Entity::Entity>& agent,
		const Vector3 destination) const
	{
		command.path.clear();
		command.path_index = 0;

		if (!agent)
			return false;

		if (const auto* map = engine.getCurrentMap())
			command.path = map->findPath(agent->getWorldPos3D(), destination);
		else
			command.path = {agent->getCenter(), {destination.x, destination.z}};

		while (!command.path.empty() &&
			   distanceSquared(command.path.front(), agent->getCenter()) <= command.request.acceptance_radius * command.request.acceptance_radius) {
			command.path.erase(command.path.begin());
		}

		return !command.path.empty();
	}

	bool AgentCommandInterface::updatePathMovement(
		AgentCommandState& command,
		const std::shared_ptr<Entity::Entity>& agent) const
	{
		if (!agent || command.path.empty())
			return false;

		while (command.path_index < command.path.size() &&
			   distanceSquared(agent->getCenter(), command.path[command.path_index]) <= command.request.acceptance_radius * command.request.acceptance_radius) {
			++command.path_index;
		}

		if (command.path_index >= command.path.size()) {
			command.path.clear();
			command.path_index = 0;
			return false;
		}

		const Vector2 next_point = command.path[command.path_index];
		agent->moveTo(next_point.x, next_point.y);
		return true;
	}

	void AgentCommandInterface::stopEntity(const std::shared_ptr<Entity::Entity>& entity) const {
		if (!entity)
			return;

		if (auto player = std::dynamic_pointer_cast<Entity::Player>(entity)) {
			player->stop();
			return;
		}

		const Vector2 position = entity->getCenter();
		entity->moveTo(position.x, position.y);
		entity->setVelocity(0.0f, 0.0f);
	}

	void AgentCommandInterface::complete(
		AgentCommandState& command,
		const AgentCommandStatus status,
		const AgentCommandFailureReason reason,
		std::string message)
	{
		command.status = status;
		command.failure_reason = reason;
		command.completed_time_seconds = _time_seconds;
		command.message = std::move(message);
		command.path.clear();
		command.path_index = 0;
	}

	void AgentCommandInterface::rememberCompleted(AgentCommandState command) {
		_completed_commands.push_back(std::move(command));
		while (_completed_commands.size() > _settings.max_completed_history)
			_completed_commands.erase(_completed_commands.begin());
	}

	bool AgentCommandInterface::isTerminal(const AgentCommandStatus status) {
		return status == AgentCommandStatus::Succeeded ||
			   status == AgentCommandStatus::Failed ||
			   status == AgentCommandStatus::Cancelled;
	}

	const char* toString(const AgentCommandType type) {
		switch (type) {
			case AgentCommandType::None:
				return "None";
			case AgentCommandType::MoveTo:
				return "MoveTo";
			case AgentCommandType::MoveToEntity:
				return "MoveToEntity";
			case AgentCommandType::Stop:
				return "Stop";
			case AgentCommandType::Attack:
				return "Attack";
			case AgentCommandType::CastAbility:
				return "CastAbility";
			case AgentCommandType::Interact:
				return "Interact";
		}

		return "Unknown";
	}

	const char* toString(const AgentCommandStatus status) {
		switch (status) {
			case AgentCommandStatus::Queued:
				return "Queued";
			case AgentCommandStatus::Running:
				return "Running";
			case AgentCommandStatus::Succeeded:
				return "Succeeded";
			case AgentCommandStatus::Failed:
				return "Failed";
			case AgentCommandStatus::Cancelled:
				return "Cancelled";
		}

		return "Unknown";
	}

	const char* toString(const AgentCommandFailureReason reason) {
		switch (reason) {
			case AgentCommandFailureReason::None:
				return "None";
			case AgentCommandFailureReason::NoAgent:
				return "NoAgent";
			case AgentCommandFailureReason::AgentUnavailable:
				return "AgentUnavailable";
			case AgentCommandFailureReason::NoTarget:
				return "NoTarget";
			case AgentCommandFailureReason::TargetUnavailable:
				return "TargetUnavailable";
			case AgentCommandFailureReason::TargetOutOfRange:
				return "TargetOutOfRange";
			case AgentCommandFailureReason::NoAbility:
				return "NoAbility";
			case AgentCommandFailureReason::AbilityOnCooldown:
				return "AbilityOnCooldown";
			case AgentCommandFailureReason::AbilityTargetMismatch:
				return "AbilityTargetMismatch";
			case AgentCommandFailureReason::ControlLocked:
				return "ControlLocked";
			case AgentCommandFailureReason::MovementRooted:
				return "MovementRooted";
			case AgentCommandFailureReason::InteractionUnavailable:
				return "InteractionUnavailable";
			case AgentCommandFailureReason::PathUnavailable:
				return "PathUnavailable";
			case AgentCommandFailureReason::CommandReplaced:
				return "CommandReplaced";
			case AgentCommandFailureReason::Cancelled:
				return "Cancelled";
		}

		return "Unknown";
	}

} // namespace Nawia::Game
