#include "Entity.h"

#include <CombatEventBus.h>
#include <EntityMovementState.h>
#include <EntityStatusController.h>
#include <Logger.h>

#include <algorithm>

namespace Nawia::Entity {

	void Entity::updateCastTelemetry(const float dt) {
		_status_controller->updateCast(dt);
	}

	void Entity::updateStatusEffects(const float dt)
	{
		if (_status_controller->updateRoot(dt)) {
			_movement_state->velocity = {0.0f, 0.0f};
			_movement_state->is_moving = false;
		}

		for (const auto& tick : _status_controller->updatePoison(dt, !isDead() && !isDying())) {
			if (isDead() || isDying())
				break;
			takeDamage(tick.damage, tick.source);
		}
	}

	void Entity::applyRoot(const float duration)
	{
		_status_controller->applyRoot(duration);
		_movement_state->velocity = {0.0f, 0.0f};
		_movement_state->is_moving = false;
	}

	void Entity::applyPoison(const float duration, const int damage_per_tick, const float tick_interval)
	{
		applyPoison(duration, damage_per_tick, tick_interval, _last_damage_source);
	}

	void Entity::applyPoison(
		const float duration,
		const int damage_per_tick,
		const float tick_interval,
		const DamageSourceContext& source_context)
	{
		_status_controller->applyPoison(duration, damage_per_tick, tick_interval, source_context);
	}

	void Entity::clearStatusEffects()
	{
		_status_controller->clearStatusEffects();
	}

	bool Entity::isMovementRooted() const
	{
		return _status_controller->isMovementRooted();
	}

	bool Entity::isPoisoned() const
	{
		return _status_controller->isPoisoned();
	}

	float Entity::getRootRemaining() const
	{
		return _status_controller->rootRemaining();
	}

	float Entity::getPoisonRemaining() const
	{
		return _status_controller->poisonRemaining();
	}

	void Entity::takeDamage(const int dmg, const DamageSourceContext& source_context)
	{
		_last_damage_source = source_context;
		takeDamage(dmg);
	}

	void Entity::takeDamage(const int dmg)
	{
		if (_is_dying) return;

		Core::Logger::debugLog("Entity " + getName() + " otrzymuje obrazenia: " + std::to_string(dmg) + ". Obecne HP: " + std::to_string(_hp));
		const int hp_before = _hp;
		const int hp_after = std::clamp(_hp - dmg, 0, _max_hp);
		const DamageSourceContext damage_source = _last_damage_source;

		if (Game::CombatEventBus* event_bus = getCombatEventBus(); event_bus && dmg > 0) {
			event_bus->emitDamageDealt(
				damage_source,
				this,
				dmg,
				hp_before,
				hp_after,
				damage_source.label);
		}

		if (_hp - dmg <= 0)
		{
			const bool killed_player_side = _type == EntityType::Player || _type == EntityType::Ally;
			_hp = 1;
			_is_dying = true;
			clearCastTelemetry();
			playAnimation(_death_anim_name, false, true, 0, true);
			setFaction(Faction::None);
			onDeathStarted();
			if (!_combat_death_event_emitted) {
				if (Game::CombatEventBus* event_bus = getCombatEventBus())
					event_bus->emitEntityKilled(damage_source, this, damage_source.label);
				_combat_death_event_emitted = true;
			}
			const auto live_damage_source = damage_source.source.lock();
			if (killed_player_side && live_damage_source && live_damage_source->healsToFullOnKill() && !live_damage_source->isDead() && !live_damage_source->isDying())
				live_damage_source->setHP(live_damage_source->getMaxHP());
			Core::Logger::debugLog("Entity " + getName() + " rozpoczela sekwencje smierci.");
		}
		else
		{
			_hp -= dmg;
		}

		_last_damage_source.label.clear();
	}

	void Entity::die()
	{
		const DamageSourceContext damage_source = _last_damage_source;
		_hp = 0;
		clearCastTelemetry();
		if (!_combat_death_event_emitted) {
			if (Game::CombatEventBus* event_bus = getCombatEventBus())
				event_bus->emitEntityKilled(damage_source, this, damage_source.label);
			_combat_death_event_emitted = true;
		}
		_last_damage_source.label.clear();
		Core::Logger::debugLog("Entity " + getName() + " zostala zabita.");
	}

	void Entity::setMaxHp(const int max_hp)
	{
		_max_hp = max_hp;
		_hp = max_hp;
		_combat_death_event_emitted = false;
	}

	void Entity::setMaxHpPreservingCurrentHp(const int max_hp)
	{
		_max_hp = max_hp;
		_hp = std::clamp(_hp, 0, _max_hp);
		if (_hp > 0)
			_combat_death_event_emitted = false;
	}

	void Entity::setHP(const int hp)
	{
		_hp = std::clamp(hp, 0, _max_hp);
		_is_dying = false;
		if (_hp > 0)
			_combat_death_event_emitted = false;
		if (_hp > 0 && _type != EntityType::Projectile)
			setFaction(_faction);
	}

	void Entity::setDeathAnimationName(std::string animation_name)
	{
		if (!animation_name.empty())
			_death_anim_name = std::move(animation_name);
	}

	nlohmann::json Entity::serializeState() const
	{
		return {
			{"name", _name},
			{"position", {{"x", _movement_state->position.x}, {"y", _movement_state->position.y}}},
			{"altitude", _movement_state->altitude},
			{"rotation", _movement_state->rotation},
			{"hp", _hp},
			{"max_hp", _max_hp},
			{"dead", isDead()},
			{"dormant", _dormant}
		};
	}

	void Entity::applyState(const nlohmann::json& state, Item::ItemDatabase* /*item_database*/)
	{
		if (!state.is_object())
			return;

		if (state.contains("position") && state["position"].is_object()) {
			_movement_state->position.x = state["position"].value("x", _movement_state->position.x);
			_movement_state->position.y = state["position"].value("y", _movement_state->position.y);
		}

		_movement_state->altitude = state.value("altitude", _movement_state->altitude);
		_movement_state->rotation = state.value("rotation", _movement_state->rotation);

		if (state.contains("max_hp") && state["max_hp"].is_number_integer())
			setMaxHp(state["max_hp"].get<int>());

		const int loaded_hp = state.value("hp", _hp);
		if (state.value("dead", false) || loaded_hp <= 0) {
			die();
			_dormant = true;
		} else {
			setHP(loaded_hp);
			_dormant = state.value("dormant", _dormant);
		}
	}

} // namespace Nawia::Entity
