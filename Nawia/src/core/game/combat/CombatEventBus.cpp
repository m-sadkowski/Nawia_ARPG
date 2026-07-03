#include "CombatEventBus.h"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace Nawia::Game {

	namespace {
		constexpr size_t MIN_STORED_EVENTS = 1;
	}

	void CombatEventBus::update(const float dt) {
		_time_seconds += std::max(0.0f, dt);
	}

	void CombatEventBus::clear() {
		_events.clear();
		_time_seconds = 0.0f;
		_next_sequence_id = 1;
	}

	std::vector<CombatEvent> CombatEventBus::getEventsSince(const float time_seconds) const {
		std::vector<CombatEvent> result;
		for (const auto& event : _events) {
			if (event.time_seconds >= time_seconds)
				result.push_back(event);
		}
		return result;
	}

	std::vector<CombatEvent> CombatEventBus::getRecentEvents(const float seconds) const {
		const float clamped_seconds = std::max(0.0f, seconds);
		return getEventsSince(_time_seconds - clamped_seconds);
	}

	CombatEventBus::SubscriptionId CombatEventBus::subscribe(Listener listener) {
		if (!listener)
			return 0;

		const SubscriptionId id = _next_subscription_id++;
		_listeners[id] = std::move(listener);
		return id;
	}

	void CombatEventBus::unsubscribe(const SubscriptionId subscription_id) {
		_listeners.erase(subscription_id);
	}

	void CombatEventBus::setMaxStoredEvents(const size_t max_stored_events) {
		_max_stored_events = std::max(MIN_STORED_EVENTS, max_stored_events);
		trimStoredEvents();
	}

	void CombatEventBus::emitDamageDealt(
		Entity::Entity* source,
		Entity::Entity* target,
		const int amount,
		const int hp_before,
		const int hp_after,
		const std::string& source_label)
	{
		emitDamageDealt(Entity::Entity::makeDamageSourceContext(source, source_label), target, amount, hp_before, hp_after, source_label);
	}

	void CombatEventBus::emitDamageDealt(
		const Entity::DamageSourceContext& source,
		Entity::Entity* target,
		const int amount,
		const int hp_before,
		const int hp_after,
		const std::string& source_label)
	{
		if (!target)
			return;

		CombatEvent event = makeEvent(CombatEventType::DamageDealt);
		event.source = makeEntityRef(source);
		event.target = makeEntityRef(target);
		event.source_label = source_label;
		event.amount = amount;
		event.hp_before = hp_before;
		event.hp_after = hp_after;
		event.lethal = hp_before > 0 && hp_after <= 0;
		event.event_position = target->getCenter();
		pushEvent(std::move(event));
	}

	void CombatEventBus::emitEntityKilled(
		Entity::Entity* killer,
		Entity::Entity* victim,
		const std::string& source_label)
	{
		emitEntityKilled(Entity::Entity::makeDamageSourceContext(killer, source_label), victim, source_label);
	}

	void CombatEventBus::emitEntityKilled(
		const Entity::DamageSourceContext& killer,
		Entity::Entity* victim,
		const std::string& source_label)
	{
		if (!victim)
			return;

		CombatEvent event = makeEvent(CombatEventType::EntityKilled);
		event.source = makeEntityRef(killer);
		event.target = makeEntityRef(victim);
		event.source_label = source_label;
		event.event_position = victim->getCenter();
		pushEvent(std::move(event));
	}

	void CombatEventBus::emitAbilityCastStarted(
		Entity::Entity* caster,
		const std::string& ability_name,
		const Vector2 target_position,
		const bool has_target_position)
	{
		if (!caster)
			return;

		CombatEvent event = makeEvent(CombatEventType::AbilityCastStarted);
		event.source = makeEntityRef(caster);
		event.source_label = ability_name;
		event.has_target_position = has_target_position;
		event.target_position = target_position;
		event.event_position = caster->getCenter();
		pushEvent(std::move(event));
	}

	void CombatEventBus::pushEvent(CombatEvent event) {
		event.sequence_id = _next_sequence_id++;
		event.time_seconds = _time_seconds;

		_events.push_back(std::move(event));
		trimStoredEvents();

		const CombatEvent event_copy = _events.back();
		std::vector<Listener> listeners;
		listeners.reserve(_listeners.size());
		for (const auto& [_, listener] : _listeners) {
			if (listener)
				listeners.push_back(listener);
		}

		for (const auto& listener : listeners)
			listener(event_copy);
	}

	CombatEvent CombatEventBus::makeEvent(const CombatEventType type) const {
		CombatEvent event;
		event.type = type;
		event.time_seconds = _time_seconds;
		return event;
	}

	CombatEntityRef CombatEventBus::makeEntityRef(Entity::Entity* entity) const {
		CombatEntityRef ref;
		if (!entity)
			return ref;

		ref.valid = true;
		ref.entity = entity->weak_from_this();
		ref.entity_id = entity->getEntityId();
		ref.name = entity->getName();
		ref.type = entity->getType();
		ref.faction = entity->getFaction();
		ref.position = entity->getCenter();
		ref.hp = entity->getHP();
		ref.max_hp = entity->getMaxHP();
		return ref;
	}

	CombatEntityRef CombatEventBus::makeEntityRef(const Entity::DamageSourceContext& context) const {
		CombatEntityRef ref;
		if (!context.valid)
			return ref;

		ref.valid = true;
		ref.entity = context.source;
		ref.entity_id = context.source_id;
		ref.name = context.source_name;
		ref.type = context.source_type;
		ref.faction = context.source_faction;
		ref.position = context.source_position;

		if (const auto live_source = context.source.lock()) {
			ref.position = live_source->getCenter();
			ref.hp = live_source->getHP();
			ref.max_hp = live_source->getMaxHP();
		}
		return ref;
	}

	void CombatEventBus::trimStoredEvents() {
		if (_events.size() <= _max_stored_events)
			return;

		const size_t overflow = _events.size() - _max_stored_events;
		_events.erase(_events.begin(), _events.begin() + static_cast<std::ptrdiff_t>(overflow));
	}

	const char* toString(const CombatEventType type) {
		switch (type) {
			case CombatEventType::DamageDealt:
				return "DamageDealt";
			case CombatEventType::EntityKilled:
				return "EntityKilled";
			case CombatEventType::AbilityCastStarted:
				return "AbilityCastStarted";
		}

		return "Unknown";
	}

} // namespace Nawia::Game
