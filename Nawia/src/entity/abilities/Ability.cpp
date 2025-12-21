#pragma once
#include "Ability.h"
#include <string>

namespace Nawia::Entity
{

	Ability::Ability(std::string name, const float cooldown, const float cast_range, AbilityTargetType target_type)
		: _name(std::move(name)), _cooldown_max(cooldown), _cast_range(cast_range), _cooldown_timer(0.0f), _target_type(target_type) {
	}

	void Ability::update(const float dt) {
		if (_cooldown_timer > 0.0f)
			_cooldown_timer -= dt;
	}

	float Ability::getCastRange() const { return _cast_range; }

	bool Ability::isReady() const { return _cooldown_timer <= 0.0f; }

	std::string Ability::getName() const { return _name; }

	AbilityTargetType Ability::getTargetType() const { return _target_type; }

} // namespace Nawia::Entity