#include "AgentPerceptionSystem.h"

#include "AgentPerceptionSupport.h"

#include <algorithm>

namespace Nawia::Game {

	void AgentPerceptionSystem::updateMemory(
		AgentPerceptionSnapshot& snapshot,
		const std::vector<AgentObservedEntity>& all_observed_entities,
		const std::shared_ptr<Entity::Entity>& agent,
		const EntityLookup& entity_lookup,
		const float time_seconds,
		const std::uint64_t frame_id)
	{
		if (!agent)
			return;

		const Entity::EntityId agent_id = AgentPerceptionSupport::entityId(agent.get());
		auto& memory = _memory_by_agent[agent_id];
		std::set<Entity::EntityId> currently_seen_ids;

		for (const auto& observed : all_observed_entities) {
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
			const auto entity = findEntityById(entity_lookup, it->first);
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

} // namespace Nawia::Game
