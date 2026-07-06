#pragma once

#include <Entity.h>

namespace Nawia::Game::AgentPerceptionSupport {

	[[nodiscard]] inline Entity::EntityId entityId(const Entity::Entity* entity) {
		return entity ? entity->getEntityId() : Entity::INVALID_ENTITY_ID;
	}

	[[nodiscard]] inline bool isCombatActorType(const Entity::EntityType type) {
		return type == Entity::EntityType::Player ||
			   type == Entity::EntityType::Ally ||
			   type == Entity::EntityType::Enemy;
	}

	[[nodiscard]] inline bool isProjectileType(const Entity::EntityType type) {
		return type == Entity::EntityType::Projectile;
	}

	[[nodiscard]] inline bool isHazardType(const Entity::EntityType type) {
		return type == Entity::EntityType::Hazard;
	}

	[[nodiscard]] inline bool isNpcType(const Entity::EntityType type) {
		return type == Entity::EntityType::NPCActor ||
			   type == Entity::EntityType::NPCStatic;
	}

	[[nodiscard]] inline bool isNeutralWorldObjectType(const Entity::EntityType type) {
		return type == Entity::EntityType::Chest ||
			   type == Entity::EntityType::Item;
	}

	[[nodiscard]] inline bool isPlayerSide(const Entity::Faction faction) {
		return faction == Entity::Faction::Player || faction == Entity::Faction::Ally;
	}

} // namespace Nawia::Game::AgentPerceptionSupport
