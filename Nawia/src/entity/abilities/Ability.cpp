#include "Ability.h"

#include <CombatEventBus.h>
#include <Entity.h>

#include <algorithm>
#include <utility>

namespace Nawia::Entity {

	Ability::Ability(std::string name,
					 const AbilityStats& stats,
					 const AbilityTargetType target_type,
					 const std::shared_ptr<Texture2D>& icon_texture)
		: _name(std::move(name)),
		  _icon_texture(icon_texture),
		  _stats(stats),
		  _target_type(target_type) {}

	void Ability::update(const float dt) {
		if (_cooldown_timer <= 0.0f)
			return;

		_cooldown_timer = std::max(0.0f, _cooldown_timer - dt);
	}

	bool Ability::isReady() const {
		return _cooldown_timer <= 0.0f;
	}

	bool Ability::canCast() const {
		return _caster != nullptr && isReady();
	}

	float Ability::getCooldownRatio() const {
		if (_stats.cooldown <= 0.0f)
			return 0.0f;

		return std::clamp(_cooldown_timer / _stats.cooldown, 0.0f, 1.0f);
	}

	bool Ability::beginCast() {
		return beginCast(0.0f, 0.0f, false);
	}

	bool Ability::beginCast(const float target_x, const float target_y) {
		return beginCast(target_x, target_y, true);
	}

	bool Ability::beginCast(const float target_x, const float target_y, const bool has_target_position) {
		if (!canCast())
			return false;

		if (auto* event_bus = Entity::getCombatEventBus()) {
			event_bus->emitAbilityCastStarted(
				_caster,
				_name,
				{target_x, target_y},
				has_target_position);
		}

		startCooldown();
		return true;
	}

	void Ability::startCooldown() {
		_cooldown_timer = std::max(0.0f, _stats.cooldown);
	}

} // namespace Nawia::Entity
