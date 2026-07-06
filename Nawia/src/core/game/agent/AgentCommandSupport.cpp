#include "AgentCommandInterface.h"

#include "AgentSystemMath.h"

#include <Engine.h>
#include <EntityManager.h>
#include <Map.h>
#include <Player.h>

#include <utility>

namespace Nawia::Game {

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

		const float acceptance_radius_sq = command.request.acceptance_radius * command.request.acceptance_radius;
		while (!command.path.empty() &&
			   AgentSystemMath::distanceSquared(command.path.front(), agent->getCenter()) <= acceptance_radius_sq) {
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

		const float acceptance_radius_sq = command.request.acceptance_radius * command.request.acceptance_radius;
		while (command.path_index < command.path.size() &&
			   AgentSystemMath::distanceSquared(agent->getCenter(), command.path[command.path_index]) <= acceptance_radius_sq) {
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

} // namespace Nawia::Game
