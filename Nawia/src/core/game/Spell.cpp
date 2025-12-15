#pragma once
#include "Spell.h"
#include <string>

namespace Nawia::Core 
{

	Spell::Spell(std::string name, const float cooldown, const float cast_range, SpellTargetType target_type) 
		: _name(std::move(name)), _cooldown_max(cooldown), _cast_range(cast_range), _cooldown_timer(0.0f), _target_type(target_type) {}

	void Spell::update(const float dt) {
	  if (_cooldown_timer > 0.0f)
	    _cooldown_timer -= dt;
	}

	bool Spell::isReady() const { return _cooldown_timer <= 0.0f; }

	std::string Spell::getName() const { return _name; }

	SpellTargetType Spell::getTargetType() const { return _target_type; }

} // namespace Nawia::Core