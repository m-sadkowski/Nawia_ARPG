#include "AgentPerceptionSystem.h"

#include "AgentPerceptionSupport.h"
#include "AgentSystemMath.h"

#include <BossTelegraphHazard.h>
#include <Chest.h>
#include <Interactable.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace Nawia::Game {

	namespace {

		[[nodiscard]] float hpRatio(const int hp, const int max_hp) {
			if (max_hp <= 0)
				return 0.0f;
			return std::clamp(static_cast<float>(hp) / static_cast<float>(max_hp), 0.0f, 1.0f);
		}

		[[nodiscard]] std::string getInteractionState(const Entity::Entity& entity, const bool interaction_available) {
			if (const auto* chest = dynamic_cast<const Entity::Chest*>(&entity)) {
				if (chest->isLocked())
					return "Locked";
				if (chest->isOpen())
					return chest->isEmpty() ? "OpenEmpty" : "Open";
				return chest->isEmpty() ? "ClosedEmpty" : "Closed";
			}

			if (dynamic_cast<const Entity::Interactable*>(&entity))
				return interaction_available ? "Available" : "Unavailable";

			return "";
		}

	}

	AgentPerceptionSnapshot AgentPerceptionSystem::buildSnapshot(
		const std::shared_ptr<Entity::Entity>& agent,
		const std::vector<std::shared_ptr<Entity::Entity>>& entities,
		const EntityLookup& entity_lookup,
		const std::vector<CombatEvent>& recent_events,
		const std::vector<MapPing>& remembered_pings,
		const MapPingManager& ping_manager,
		const float time_seconds,
		const std::uint64_t frame_id)
	{
		AgentPerceptionSnapshot snapshot;
		snapshot.frame_id = frame_id;
		snapshot.time_seconds = time_seconds;
		snapshot.perception_radius = _settings.perception_radius;
		snapshot.event_memory_seconds = _settings.event_memory_seconds;
		snapshot.lost_memory_seconds = _settings.lost_memory_seconds;
		snapshot.terminal_lost_memory_seconds = _settings.terminal_lost_memory_seconds;
		snapshot.self = makeEntitySnapshot(agent);

		const auto current_target = agent->getTarget();
		const Entity::EntityId current_target_id = AgentPerceptionSupport::entityId(current_target.get());
		if (current_target && current_target->isPerceptionVisible())
			snapshot.current_target = makeEntitySnapshot(current_target);

		if (const auto last_damage_source = agent->getLastDamageSource())
			snapshot.last_damage_source = makeEntitySnapshot(last_damage_source);

		const auto& abilities = agent->getAbilities();
		snapshot.abilities.reserve(abilities.size());
		for (size_t index = 0; index < abilities.size(); ++index) {
			if (abilities[index])
				snapshot.abilities.push_back(makeAbilitySnapshot(abilities[index], static_cast<int>(index)));
		}

		for (const auto& ping : ping_manager.getActivePings()) {
			if (isPingRelevantToAgent(*agent, ping))
				snapshot.visible_pings.push_back(ping);
		}

		for (const auto& ping : remembered_pings) {
			if (isPingRelevantToAgent(*agent, ping))
				snapshot.remembered_pings.push_back(ping);
		}

		const Vector2 agent_position = agent->getCenter();
		const float radius_sq = _settings.perception_radius * _settings.perception_radius;
		std::vector<AgentObservedEntity> all_observed_entities;

		for (const auto& entity : entities) {
			if (entity == agent || !isPerceivableEntity(entity))
				continue;

			const Vector2 entity_position = entity->getCenter();
			const float distance_sq = AgentSystemMath::distanceSquared(agent_position, entity_position);
			if (distance_sq > radius_sq)
				continue;

			const float distance = std::sqrt(distance_sq);
			AgentObservedEntity observed;
			observed.entity = makeEntitySnapshot(entity);
			observed.relation = getRelation(*agent, *entity);
			observed.distance = distance;
			if (distance > std::numeric_limits<float>::epsilon()) {
				observed.direction = {
					(entity_position.x - agent_position.x) / distance,
					(entity_position.y - agent_position.y) / distance
				};
			}
			observed.is_current_target = observed.entity.entity_id == current_target_id;

			if (observed.relation == AgentRelation::Enemy)
				++snapshot.nearby_enemy_count;
			else if (observed.relation == AgentRelation::Ally)
				++snapshot.nearby_ally_count;
			else if (observed.relation == AgentRelation::Neutral)
				++snapshot.nearby_neutral_count;

			if (AgentPerceptionSupport::isNpcType(observed.entity.type))
				++snapshot.nearby_npc_count;

			if (observed.entity.type == Entity::EntityType::Projectile)
				++snapshot.nearby_projectile_count;

			if (observed.entity.type == Entity::EntityType::Hazard)
				++snapshot.nearby_hazard_count;

			all_observed_entities.push_back(std::move(observed));
		}

		std::sort(all_observed_entities.begin(), all_observed_entities.end(), [](const auto& left, const auto& right) {
			return left.distance < right.distance;
		});
		snapshot.observed_entities = all_observed_entities;
		if (snapshot.observed_entities.size() > _settings.max_observed_entities)
			snapshot.observed_entities.resize(_settings.max_observed_entities);

		updateMemory(snapshot, all_observed_entities, agent, entity_lookup, time_seconds, frame_id);

		for (const auto& event : recent_events) {
			if (!isEventRelevantToAgent(event, snapshot.self.entity_id, agent_position))
				continue;

			if (snapshot.recent_combat_events.size() >= _settings.max_recent_events)
				snapshot.recent_combat_events.erase(snapshot.recent_combat_events.begin());

			snapshot.recent_combat_events.push_back(event);
		}

		return snapshot;
	}

	AgentEntitySnapshot AgentPerceptionSystem::makeEntitySnapshot(const std::shared_ptr<Entity::Entity>& entity) const {
		AgentEntitySnapshot snapshot;
		if (!entity)
			return snapshot;

		snapshot.valid = true;
		snapshot.entity = entity;
		snapshot.entity_id = AgentPerceptionSupport::entityId(entity.get());
		snapshot.name = entity->getName();
		snapshot.type = entity->getType();
		snapshot.faction = entity->getFaction();
		snapshot.position = entity->getCenter();
		snapshot.velocity = entity->getVelocity();
		snapshot.hp = entity->getHP();
		snapshot.max_hp = entity->getMaxHP();
		snapshot.hp_ratio = hpRatio(snapshot.hp, snapshot.max_hp);
		snapshot.alive = !entity->isDead();
		snapshot.dying = entity->isDying();
		snapshot.dormant = entity->isDormant();
		snapshot.visible = entity->isPerceptionVisible();
		if (auto* interactable = dynamic_cast<Entity::Interactable*>(entity.get())) {
			snapshot.interactable = true;
			snapshot.interaction_available = interactable->canInteract();
			snapshot.interaction_range = interactable->getInteractionRange();
			snapshot.interaction_state = getInteractionState(*entity, snapshot.interaction_available);
		}
		snapshot.moving = entity->isMoving();
		snapshot.rooted = entity->isMovementRooted();
		snapshot.poisoned = entity->isPoisoned();
		snapshot.root_remaining = entity->getRootRemaining();
		snapshot.poison_remaining = entity->getPoisonRemaining();

		const Entity::EntityCastState& cast = entity->getCastState();
		snapshot.casting = cast.active;
		snapshot.cast_name = cast.name;
		snapshot.cast_duration = cast.duration_seconds;
		snapshot.cast_remaining = cast.remaining_seconds;
		snapshot.cast_interruptible = cast.interruptible;

		if (const auto* hazard = dynamic_cast<const Entity::BossTelegraphHazard*>(entity.get())) {
			snapshot.hazard = true;
			snapshot.hazard_phase = hazard->getHazardPhaseName();
			snapshot.hazard_radius = hazard->getRadius();
			snapshot.hazard_current_radius = hazard->getCurrentRadius();
			snapshot.hazard_time_to_activate = hazard->getTimeToActivate();
			snapshot.hazard_remaining = hazard->getRemainingActiveSeconds();
			snapshot.hazard_damage_per_tick = hazard->getDamagePerTick();
			snapshot.hazard_tick_interval = hazard->getTickInterval();
			snapshot.hazard_knock_down_player_on_hit = hazard->knocksDownPlayerOnHit();
			snapshot.hazard_expanding_wave = hazard->isExpandingWave();
			snapshot.hazard_source_entity_id = hazard->getSourceEntityId();
		}
		return snapshot;
	}

	AgentAbilitySnapshot AgentPerceptionSystem::makeAbilitySnapshot(
		const std::shared_ptr<Entity::Ability>& ability,
		const int slot) const
	{
		AgentAbilitySnapshot snapshot;
		if (!ability)
			return snapshot;

		const Entity::AbilityStats& stats = ability->getStats();
		snapshot.slot = slot;
		snapshot.name = ability->getName();
		snapshot.target_type = ability->getTargetType();
		snapshot.ready = ability->isReady();
		snapshot.can_cast = ability->canCast();
		snapshot.cooldown_remaining = ability->getCooldownTimer();
		snapshot.cooldown_ratio = ability->getCooldownRatio();
		snapshot.cooldown = stats.cooldown;
		snapshot.cast_range = stats.cast_range;
		snapshot.duration = stats.duration;
		snapshot.projectile_speed = stats.projectile_speed;
		snapshot.hitbox_radius = stats.hitbox_radius;
		snapshot.damage = stats.damage;
		return snapshot;
	}

} // namespace Nawia::Game
