#pragma once
#include "Ability.h"
#include "Entity.h"

#include <string>

namespace Nawia::Entity
{

	Ability::Ability(std::string name, const AbilityStats& stats, AbilityTargetType target_type, const std::shared_ptr<Texture2D>& icon_texture)
		: _name(std::move(name)), _stats(stats), _target_type(target_type), _icon_texture(icon_texture), _cooldown_timer(0.0f), _caster(nullptr) {}

	void Ability::update(const float dt) 
	{
		if (_cooldown_timer > 0.0f)
			_cooldown_timer -= dt;
	}

	float Ability::getCastRange() const
	{
		return _stats.cast_range;
	}

	bool Ability::isReady() const
	{
		return _cooldown_timer <= 0.0f;
	}

	std::string Ability::getName() const
	{
		return _name;
	}

	AbilityTargetType Ability::getTargetType() const
	{
		return _target_type;
	}

} // namespace Nawia::Entity