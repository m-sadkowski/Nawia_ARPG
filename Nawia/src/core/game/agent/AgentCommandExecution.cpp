#include "AgentCommandInterface.h"

#include "AgentSystemMath.h"

#include <Ability.h>
#include <Engine.h>
#include <Interactable.h>

#include <algorithm>
#include <cmath>

namespace Nawia::Game {

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

		const float acceptance_radius_sq = command.request.acceptance_radius * command.request.acceptance_radius;
		if (AgentSystemMath::distanceToBoxSquared(*target, agent->getCenter()) <= acceptance_radius_sq) {
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
		const float acceptance_radius_sq = command.request.acceptance_radius * command.request.acceptance_radius;
		if (AgentSystemMath::distanceSquared(agent->getCenter(), destination) <= acceptance_radius_sq) {
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

		if (AgentSystemMath::distanceSquared(agent->getCenter(), destination) <= acceptance_radius_sq) {
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

		const float distance = std::sqrt(AgentSystemMath::distanceSquared(agent->getCenter(), target->getCenter()));
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
		if (cast_range > 0.0f && AgentSystemMath::distanceSquared(agent->getCenter(), target_position) > cast_range * cast_range) {
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
		if (AgentSystemMath::distanceToBoxSquared(*target, agent->getCenter()) <= interaction_range_sq) {
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

} // namespace Nawia::Game
