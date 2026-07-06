#include "AgentPerceptionSystem.h"

#include "AgentPerceptionSupport.h"
#include "AgentSystemMath.h"

namespace Nawia::Game {

	bool AgentPerceptionSystem::isAgentCandidate(const std::shared_ptr<Entity::Entity>& entity) const {
		if (!entity || !entity->isPerceptionVisible())
			return false;

		const Entity::EntityType type = entity->getType();
		if (type == Entity::EntityType::Player)
			return _settings.include_player;
		if (type == Entity::EntityType::Ally)
			return _settings.include_allies;
		if (type == Entity::EntityType::Enemy)
			return _settings.include_enemies;

		return false;
	}

	bool AgentPerceptionSystem::isPerceivableEntity(const std::shared_ptr<Entity::Entity>& entity) const {
		if (!entity || !entity->isPerceptionVisible())
			return false;

		const Entity::EntityType type = entity->getType();
		if (AgentPerceptionSupport::isCombatActorType(type))
			return true;
		if (_settings.include_projectiles && AgentPerceptionSupport::isProjectileType(type))
			return true;
		if (AgentPerceptionSupport::isHazardType(type))
			return true;
		if (_settings.include_npcs && AgentPerceptionSupport::isNpcType(type))
			return true;
		if (_settings.include_neutral_entities && AgentPerceptionSupport::isNeutralWorldObjectType(type))
			return true;

		return false;
	}

	AgentRelation AgentPerceptionSystem::getRelation(
		const Entity::Entity& observer,
		const Entity::Entity& observed) const
	{
		if (&observer == &observed)
			return AgentRelation::Self;

		const Entity::EntityType observed_type = observed.getType();
		if (observed_type == Entity::EntityType::Projectile)
			return AgentRelation::Unknown;
		if (observed_type == Entity::EntityType::Hazard && observed.getFaction() == Entity::Faction::Enemy)
			return AgentPerceptionSupport::isPlayerSide(observer.getFaction()) ? AgentRelation::Enemy : AgentRelation::Ally;
		if (AgentPerceptionSupport::isNpcType(observed_type) || AgentPerceptionSupport::isNeutralWorldObjectType(observed_type))
			return AgentRelation::Neutral;

		const Entity::Faction observer_faction = observer.getFaction();
		const Entity::Faction observed_faction = observed.getFaction();

		if (observed_faction == Entity::Faction::Neutral)
			return AgentRelation::Neutral;

		if (observer_faction == observed_faction)
			return AgentRelation::Ally;

		if (observer_faction == Entity::Faction::None || observed_faction == Entity::Faction::None)
			return AgentRelation::Unknown;

		if ((AgentPerceptionSupport::isPlayerSide(observer_faction) && observed_faction == Entity::Faction::Enemy) ||
			(observer_faction == Entity::Faction::Enemy && AgentPerceptionSupport::isPlayerSide(observed_faction))) {
			return AgentRelation::Enemy;
		}

		return AgentRelation::Unknown;
	}

	bool AgentPerceptionSystem::isEventRelevantToAgent(
		const CombatEvent& event,
		const Entity::EntityId agent_id,
		const Vector2 agent_position) const
	{
		if (event.source.entity_id == agent_id || event.target.entity_id == agent_id)
			return true;

		const float radius_sq = _settings.perception_radius * _settings.perception_radius;
		if (AgentSystemMath::distanceSquared(agent_position, event.event_position) <= radius_sq)
			return true;
		if (event.source.valid && AgentSystemMath::distanceSquared(agent_position, event.source.position) <= radius_sq)
			return true;
		if (event.target.valid && AgentSystemMath::distanceSquared(agent_position, event.target.position) <= radius_sq)
			return true;

		return false;
	}

	bool AgentPerceptionSystem::isPingRelevantToAgent(const Entity::Entity& agent, const MapPing& ping) const {
		if (!ping.source.valid)
			return false;

		return AgentPerceptionSupport::isPlayerSide(agent.getFaction()) && AgentPerceptionSupport::isPlayerSide(ping.source.faction);
	}

	std::shared_ptr<Entity::Entity> AgentPerceptionSystem::findEntityById(
		const EntityLookup& entity_lookup,
		const Entity::EntityId entity_id) const
	{
		const auto it = entity_lookup.find(entity_id);
		return it != entity_lookup.end() ? it->second : nullptr;
	}

	std::string AgentPerceptionSystem::getDisappearanceReason(
		const std::shared_ptr<Entity::Entity>& entity,
		const Vector2 agent_position) const
	{
		if (!entity)
			return "Removed";
		if (entity->isDead())
			return "Dead";
		if (entity->isDying())
			return "Dying";
		if (entity->isDormant())
			return "Dormant";
		if (!entity->isPerceptionVisible())
			return "NotVisible";
		if (AgentSystemMath::distanceSquared(agent_position, entity->getCenter()) > _settings.perception_radius * _settings.perception_radius)
			return "OutOfRange";

		return "NotObserved";
	}

	const char* toString(const AgentRelation relation) {
		switch (relation) {
			case AgentRelation::Self:
				return "Self";
			case AgentRelation::Ally:
				return "Ally";
			case AgentRelation::Enemy:
				return "Enemy";
			case AgentRelation::Neutral:
				return "Neutral";
			case AgentRelation::Unknown:
				return "Unknown";
		}

		return "Unknown";
	}

} // namespace Nawia::Game
