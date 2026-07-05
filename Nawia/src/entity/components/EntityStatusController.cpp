#include "EntityStatusController.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Nawia::Entity {

	void EntityStatusController::beginCast(std::string cast_name, const float duration_seconds, const bool interruptible)
	{
		_cast_state.active = !cast_name.empty();
		_cast_state.name = std::move(cast_name);
		_cast_state.duration_seconds = std::max(0.0f, duration_seconds);
		_cast_state.remaining_seconds = _cast_state.duration_seconds;
		_cast_state.interruptible = interruptible;
	}

	void EntityStatusController::clearCast()
	{
		_cast_state = {};
	}

	void EntityStatusController::updateCast(const float dt)
	{
		if (!_cast_state.active)
			return;

		_cast_state.remaining_seconds = std::max(0.0f, _cast_state.remaining_seconds - std::max(0.0f, dt));
		if (_cast_state.remaining_seconds <= 0.0f)
			_cast_state.active = false;
	}

	void EntityStatusController::applyRoot(const float duration)
	{
		_root_timer = std::max(_root_timer, duration);
	}

	bool EntityStatusController::updateRoot(const float dt)
	{
		if (_root_timer <= 0.0f)
			return false;

		_root_timer = std::max(0.0f, _root_timer - dt);
		return _root_timer > 0.0f;
	}

	void EntityStatusController::applyPoison(
		const float duration,
		const int damage_per_tick,
		const float tick_interval,
		const DamageSourceContext& source_context)
	{
		if (duration <= 0.0f || damage_per_tick <= 0)
			return;

		_poison_timer = std::max(_poison_timer, duration);
		_poison_tick_interval = std::max(0.05f, tick_interval);
		_poison_tick_timer = std::min(
			_poison_tick_timer > 0.0f ? _poison_tick_timer : _poison_tick_interval,
			_poison_tick_interval);
		_poison_damage_per_tick = std::max(_poison_damage_per_tick, damage_per_tick);
		_poison_damage_source = source_context;
	}

	std::vector<EntityStatusController::PoisonTick> EntityStatusController::updatePoison(
		const float dt,
		const bool can_apply_damage)
	{
		if (_poison_timer <= 0.0f || _poison_damage_per_tick <= 0 || !can_apply_damage)
			return {};

		const float poison_timer_before_update = _poison_timer;
		_poison_timer = std::max(0.0f, _poison_timer - dt);
		_poison_tick_timer -= dt;

		const float tick_interval = std::max(0.05f, _poison_tick_interval);
		const int max_ticks_this_update = static_cast<int>(std::ceil(poison_timer_before_update / tick_interval));

		std::vector<PoisonTick> ticks;
		ticks.reserve(static_cast<size_t>(std::max(0, max_ticks_this_update)));
		while (_poison_tick_timer <= 0.0f &&
			   static_cast<int>(ticks.size()) < max_ticks_this_update &&
			   poison_timer_before_update > 0.0f) {
			ticks.push_back({_poison_damage_per_tick, _poison_damage_source});
			_poison_tick_timer += tick_interval;
		}

		return ticks;
	}

	void EntityStatusController::clearStatusEffects()
	{
		_root_timer = 0.0f;
		_poison_timer = 0.0f;
		_poison_tick_timer = 0.0f;
		_poison_tick_interval = 1.0f;
		_poison_damage_per_tick = 0;
		_poison_damage_source = {};
	}

}
