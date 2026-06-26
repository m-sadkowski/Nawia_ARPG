#pragma once

#include <Entity.h>

#include <raylib.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Nawia::Game {

	/**
	 * @enum CombatEventType
	 * @brief Runtime combat facts emitted by low-level gameplay systems.
	 *
	 * These events are intentionally factual. They do not decide threat, roles,
	 * tactics, or evaluation metrics; thesis systems can build those layers on
	 * top of this stream.
	 */
	enum class CombatEventType {
		DamageDealt,
		EntityKilled,
		AbilityCastStarted
	};

	/**
	 * @brief Lightweight snapshot of an entity at event time.
	 *
	 * The weak pointer is useful when the entity is still alive, while copied
	 * fields keep the event readable after the entity is removed from the world.
	 */
	struct CombatEntityRef {
		bool valid = false;
		std::weak_ptr<Entity::Entity> entity;
		std::uintptr_t runtime_id = 0;
		std::string name;
		Entity::EntityType type = Entity::EntityType::None;
		Entity::Faction faction = Entity::Faction::None;
		Vector2 position = {0.0f, 0.0f};
		int hp = 0;
		int max_hp = 0;
	};

	/**
	 * @brief Single combat fact recorded by CombatEventBus.
	 */
	struct CombatEvent {
		std::uint64_t sequence_id = 0;
		float time_seconds = 0.0f;
		CombatEventType type = CombatEventType::DamageDealt;

		CombatEntityRef source;
		CombatEntityRef target;

		std::string source_label;
		int amount = 0;
		int hp_before = 0;
		int hp_after = 0;
		bool lethal = false;

		bool has_target_position = false;
		Vector2 target_position = {0.0f, 0.0f};
		Vector2 event_position = {0.0f, 0.0f};
	};

	/**
	 * @class CombatEventBus
	 * @brief Stores and broadcasts low-level combat events.
	 *
	 * The bus supports two access patterns:
	 * - subscribe for immediate reactions,
	 * - query recent events for perception, analysis, or diagnostics.
	 */
	class CombatEventBus {
	public:
		using Listener = std::function<void(const CombatEvent&)>;
		using SubscriptionId = std::uint64_t;

		void update(float dt);
		void clear();

		[[nodiscard]] float getTimeSeconds() const { return _time_seconds; }
		[[nodiscard]] const std::vector<CombatEvent>& getEvents() const { return _events; }
		[[nodiscard]] std::vector<CombatEvent> getEventsSince(float time_seconds) const;
		[[nodiscard]] std::vector<CombatEvent> getRecentEvents(float seconds) const;

		SubscriptionId subscribe(Listener listener);
		void unsubscribe(SubscriptionId subscription_id);

		void setMaxStoredEvents(size_t max_stored_events);
		[[nodiscard]] size_t getMaxStoredEvents() const { return _max_stored_events; }

		void emitDamageDealt(
			Entity::Entity* source,
			Entity::Entity* target,
			int amount,
			int hp_before,
			int hp_after,
			const std::string& source_label);

		void emitEntityKilled(
			Entity::Entity* killer,
			Entity::Entity* victim,
			const std::string& source_label);

		void emitAbilityCastStarted(
			Entity::Entity* caster,
			const std::string& ability_name,
			Vector2 target_position,
			bool has_target_position);

	private:
		void pushEvent(CombatEvent event);
		[[nodiscard]] CombatEvent makeEvent(CombatEventType type) const;
		[[nodiscard]] CombatEntityRef makeEntityRef(Entity::Entity* entity) const;
		void trimStoredEvents();

		float _time_seconds = 0.0f;
		std::uint64_t _next_sequence_id = 1;
		std::uint64_t _next_subscription_id = 1;
		size_t _max_stored_events = 1024;
		std::vector<CombatEvent> _events;
		std::map<SubscriptionId, Listener> _listeners;
	};

	[[nodiscard]] const char* toString(CombatEventType type);

} // namespace Nawia::Game
