#include "AgentPerceptionSystem.h"

#include <BossTelegraphHazard.h>
#include <Chest.h>
#include <EntityManager.h>
#include <Interactable.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace Nawia::Game {

	namespace {
		[[nodiscard]] Entity::EntityId entityId(const Entity::Entity* entity) {
			return entity ? entity->getEntityId() : Entity::INVALID_ENTITY_ID;
		}

		[[nodiscard]] float distanceSquared(const Vector2 first, const Vector2 second) {
			const float dx = second.x - first.x;
			const float dy = second.y - first.y;
			return dx * dx + dy * dy;
		}

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

		[[nodiscard]] bool isCombatActorType(const Entity::EntityType type) {
			return type == Entity::EntityType::Player ||
				   type == Entity::EntityType::Ally ||
				   type == Entity::EntityType::Enemy;
		}

		[[nodiscard]] bool isProjectileType(const Entity::EntityType type) {
			return type == Entity::EntityType::Projectile;
		}

		[[nodiscard]] bool isHazardType(const Entity::EntityType type) {
			return type == Entity::EntityType::Hazard;
		}

		[[nodiscard]] bool isNpcType(const Entity::EntityType type) {
			return type == Entity::EntityType::NPCActor ||
				   type == Entity::EntityType::NPCStatic;
		}

		[[nodiscard]] bool isNeutralWorldObjectType(const Entity::EntityType type) {
			return type == Entity::EntityType::Chest ||
				   type == Entity::EntityType::Item;
		}

		[[nodiscard]] bool isPlayerSide(const Entity::Faction faction) {
			return faction == Entity::Faction::Player || faction == Entity::Faction::Ally;
		}
	}

	void AgentPerceptionSystem::update(
		const Core::EntityManager& entity_manager,
		const CombatEventBus& combat_event_bus,
		const MapPingManager& ping_manager)
	{
		_snapshots.clear();
		const auto& entities = entity_manager.getEntities();
		const std::vector<CombatEvent> recent_events = combat_event_bus.getRecentEvents(_settings.event_memory_seconds);
		const std::uint64_t frame_id = _next_frame_id++;
		_last_frame_id = frame_id;
		std::set<Entity::EntityId> active_agent_ids;

		for (const auto& entity : entities) {
			if (!isAgentCandidate(entity))
				continue;

			auto snapshot = buildSnapshot(entity, entities, recent_events, ping_manager, combat_event_bus.getTimeSeconds(), frame_id);
			active_agent_ids.insert(snapshot.self.entity_id);
			_snapshots.push_back(std::move(snapshot));
		}

		for (auto it = _memory_by_agent.begin(); it != _memory_by_agent.end();) {
			if (active_agent_ids.contains(it->first))
				++it;
			else
				it = _memory_by_agent.erase(it);
		}
	}

	void AgentPerceptionSystem::clear() {
		_snapshots.clear();
		_memory_by_agent.clear();
		_last_frame_id = 0;
		_next_frame_id = 1;
	}

	void AgentPerceptionSystem::setSettings(const Settings& settings) {
		_settings = settings;
		_settings.perception_radius = std::max(0.0f, _settings.perception_radius);
		_settings.event_memory_seconds = std::max(0.0f, _settings.event_memory_seconds);
		_settings.lost_memory_seconds = std::max(0.0f, _settings.lost_memory_seconds);
		_settings.terminal_lost_memory_seconds = std::max(0.0f, _settings.terminal_lost_memory_seconds);
		_settings.max_observed_entities = std::max<size_t>(1, _settings.max_observed_entities);
		_settings.max_lost_entities = std::max<size_t>(1, _settings.max_lost_entities);
		_settings.max_recent_events = std::max<size_t>(1, _settings.max_recent_events);
	}

	const AgentPerceptionSnapshot* AgentPerceptionSystem::findSnapshot(const Entity::EntityId entity_id) const {
		for (const auto& snapshot : _snapshots) {
			if (snapshot.self.entity_id == entity_id)
				return &snapshot;
		}
		return nullptr;
	}

	const AgentPerceptionSnapshot* AgentPerceptionSystem::findSnapshot(const Entity::Entity& entity) const {
		return findSnapshot(entityId(&entity));
	}

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
		if (isCombatActorType(type))
			return true;
		if (_settings.include_projectiles && isProjectileType(type))
			return true;
		if (isHazardType(type))
			return true;
		if (_settings.include_npcs && isNpcType(type))
			return true;
		if (_settings.include_neutral_entities && isNeutralWorldObjectType(type))
			return true;

		return false;
	}

	AgentPerceptionSnapshot AgentPerceptionSystem::buildSnapshot(
		const std::shared_ptr<Entity::Entity>& agent,
		const std::vector<std::shared_ptr<Entity::Entity>>& entities,
		const std::vector<CombatEvent>& recent_events,
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
		const Entity::EntityId current_target_id = entityId(current_target.get());
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

		for (const auto& ping : ping_manager.getRememberedPings()) {
			if (isPingRelevantToAgent(*agent, ping))
				snapshot.remembered_pings.push_back(ping);
		}

		const Vector2 agent_position = agent->getCenter();
		const float radius_sq = _settings.perception_radius * _settings.perception_radius;

		for (const auto& entity : entities) {
			if (entity == agent || !isPerceivableEntity(entity))
				continue;

			const Vector2 entity_position = entity->getCenter();
			const float distance_sq = distanceSquared(agent_position, entity_position);
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

			if (isNpcType(observed.entity.type))
				++snapshot.nearby_npc_count;

			if (observed.entity.type == Entity::EntityType::Projectile)
				++snapshot.nearby_projectile_count;

			if (observed.entity.type == Entity::EntityType::Hazard)
				++snapshot.nearby_hazard_count;

			snapshot.observed_entities.push_back(std::move(observed));
		}

		std::sort(snapshot.observed_entities.begin(), snapshot.observed_entities.end(), [](const auto& left, const auto& right) {
			return left.distance < right.distance;
		});
		if (snapshot.observed_entities.size() > _settings.max_observed_entities)
			snapshot.observed_entities.resize(_settings.max_observed_entities);

		updateMemory(snapshot, agent, entities, time_seconds, frame_id);

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
		snapshot.entity_id = entityId(entity.get());
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
			return isPlayerSide(observer.getFaction()) ? AgentRelation::Enemy : AgentRelation::Ally;
		if (isNpcType(observed_type) || isNeutralWorldObjectType(observed_type))
			return AgentRelation::Neutral;

		const Entity::Faction observer_faction = observer.getFaction();
		const Entity::Faction observed_faction = observed.getFaction();

		if (observed_faction == Entity::Faction::Neutral)
			return AgentRelation::Neutral;

		if (observer_faction == observed_faction)
			return AgentRelation::Ally;

		if (observer_faction == Entity::Faction::None || observed_faction == Entity::Faction::None)
			return AgentRelation::Unknown;

		if ((isPlayerSide(observer_faction) && observed_faction == Entity::Faction::Enemy) ||
			(observer_faction == Entity::Faction::Enemy && isPlayerSide(observed_faction))) {
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
		if (distanceSquared(agent_position, event.event_position) <= radius_sq)
			return true;
		if (event.source.valid && distanceSquared(agent_position, event.source.position) <= radius_sq)
			return true;
		if (event.target.valid && distanceSquared(agent_position, event.target.position) <= radius_sq)
			return true;

		return false;
	}

	bool AgentPerceptionSystem::isPingRelevantToAgent(const Entity::Entity& agent, const MapPing& ping) const {
		if (!ping.source.valid)
			return false;

		return isPlayerSide(agent.getFaction()) && isPlayerSide(ping.source.faction);
	}

	void AgentPerceptionSystem::updateMemory(
		AgentPerceptionSnapshot& snapshot,
		const std::shared_ptr<Entity::Entity>& agent,
		const std::vector<std::shared_ptr<Entity::Entity>>& entities,
		const float time_seconds,
		const std::uint64_t frame_id)
	{
		if (!agent)
			return;

		const Entity::EntityId agent_id = entityId(agent.get());
		auto& memory = _memory_by_agent[agent_id];
		std::set<Entity::EntityId> currently_seen_ids;

		for (const auto& observed : snapshot.observed_entities) {
			const Entity::EntityId observed_entity_id = observed.entity.entity_id;
			currently_seen_ids.insert(observed_entity_id);

			auto& record = memory[observed_entity_id];
			record.last_known_entity = observed.entity;
			record.relation = observed.relation;
			record.last_seen_time_seconds = time_seconds;
			record.last_seen_frame_id = frame_id;
			record.was_current_target = observed.is_current_target;
			record.terminal = false;
			record.terminal_time_seconds = 0.0f;
			record.terminal_reason.clear();
		}

		const Vector2 agent_position = agent->getCenter();
		for (auto it = memory.begin(); it != memory.end();) {
			if (currently_seen_ids.contains(it->first)) {
				++it;
				continue;
			}

			const float seconds_since_seen = std::max(0.0f, time_seconds - it->second.last_seen_time_seconds);
			const auto entity = findEntityById(entities, it->first);
			if (!entity || entity->isDead() || entity->isDying()) {
				if (!it->second.terminal) {
					it->second.terminal = true;
					it->second.terminal_time_seconds = time_seconds;
					it->second.terminal_reason = getDisappearanceReason(entity, agent_position);
					if (entity)
						it->second.last_known_entity = makeEntitySnapshot(entity);
				}

				const float seconds_since_terminal = std::max(0.0f, time_seconds - it->second.terminal_time_seconds);
				if (seconds_since_terminal > _settings.terminal_lost_memory_seconds) {
					it = memory.erase(it);
					continue;
				}

				AgentLostEntity lost;
				lost.last_known_entity = it->second.last_known_entity;
				lost.relation = it->second.relation;
				lost.last_known_position = it->second.last_known_entity.position;
				lost.last_seen_time_seconds = it->second.last_seen_time_seconds;
				lost.seconds_since_seen = seconds_since_seen;
				lost.was_current_target = it->second.was_current_target;
				lost.disappearance_reason = it->second.terminal_reason;
				snapshot.lost_entities.push_back(std::move(lost));
				++it;
				continue;
			}

			if (seconds_since_seen > _settings.lost_memory_seconds) {
				it = memory.erase(it);
				continue;
			}

			AgentLostEntity lost;
			lost.last_known_entity = it->second.last_known_entity;
			lost.relation = it->second.relation;
			lost.last_known_position = it->second.last_known_entity.position;
			lost.last_seen_time_seconds = it->second.last_seen_time_seconds;
			lost.seconds_since_seen = seconds_since_seen;
			lost.was_current_target = it->second.was_current_target;
			lost.disappearance_reason = getDisappearanceReason(entity, agent_position);
			snapshot.lost_entities.push_back(std::move(lost));
			++it;
		}

		std::sort(snapshot.lost_entities.begin(), snapshot.lost_entities.end(), [](const auto& left, const auto& right) {
			return left.seconds_since_seen < right.seconds_since_seen;
		});
		if (snapshot.lost_entities.size() > _settings.max_lost_entities)
			snapshot.lost_entities.resize(_settings.max_lost_entities);

		snapshot.lost_entity_count = snapshot.lost_entities.size();
	}

	std::shared_ptr<Entity::Entity> AgentPerceptionSystem::findEntityById(
		const std::vector<std::shared_ptr<Entity::Entity>>& entities,
		const Entity::EntityId entity_id) const
	{
		for (const auto& entity : entities) {
			if (entityId(entity.get()) == entity_id)
				return entity;
		}
		return nullptr;
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
		if (distanceSquared(agent_position, entity->getCenter()) > _settings.perception_radius * _settings.perception_radius)
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
