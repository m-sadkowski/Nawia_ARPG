#include "AgentPerceptionSystem.h"

#include "AgentPerceptionSupport.h"

#include <EntityManager.h>

#include <algorithm>

namespace Nawia::Game {

	void AgentPerceptionSystem::update(
		const Core::EntityManager& entity_manager,
		const CombatEventBus& combat_event_bus,
		const MapPingManager& ping_manager)
	{
		_snapshots.clear();
		const auto& entities = entity_manager.getEntities();
		const std::vector<CombatEvent> recent_events = combat_event_bus.getRecentEvents(_settings.event_memory_seconds);
		const std::vector<MapPing> remembered_pings = ping_manager.getRememberedPings();
		EntityLookup entity_lookup;
		entity_lookup.reserve(entities.size());
		for (const auto& entity : entities) {
			const Entity::EntityId id = AgentPerceptionSupport::entityId(entity.get());
			if (id != Entity::INVALID_ENTITY_ID)
				entity_lookup[id] = entity;
		}

		const std::uint64_t frame_id = _next_frame_id++;
		_last_frame_id = frame_id;
		std::set<Entity::EntityId> active_agent_ids;

		for (const auto& entity : entities) {
			if (!isAgentCandidate(entity))
				continue;

			auto snapshot = buildSnapshot(
				entity,
				entities,
				entity_lookup,
				recent_events,
				remembered_pings,
				ping_manager,
				combat_event_bus.getTimeSeconds(),
				frame_id);
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
		return findSnapshot(AgentPerceptionSupport::entityId(&entity));
	}

} // namespace Nawia::Game
