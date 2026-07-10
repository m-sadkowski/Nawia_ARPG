#include "EntityAbilityController.h"

#include <Ability.h>
#include <Entity.h>

namespace Nawia::Entity {

	void EntityAbilityController::addAbility(Entity& owner, const std::shared_ptr<Ability>& ability)
	{
		if (!ability)
			return;

		ability->setCaster(&owner);
		_abilities.push_back(ability);
	}

	void EntityAbilityController::setAbility(Entity& owner, const int index, const std::shared_ptr<Ability>& ability)
	{
		if (index < 0 || !ability)
			return;

		ability->setCaster(&owner);
		const auto ability_index = static_cast<size_t>(index);
		if (_abilities.size() <= ability_index)
			_abilities.resize(ability_index + 1);

		_abilities[ability_index] = ability;
	}

	std::shared_ptr<Ability> EntityAbilityController::getAbility(const int index) const
	{
		if (index < 0)
			return nullptr;

		const auto ability_index = static_cast<size_t>(index);
		if (ability_index < _abilities.size())
			return _abilities[ability_index];

		return nullptr;
	}

	void EntityAbilityController::updateAbilities(const float dt) const
	{
		for (const auto& ability : _abilities) {
			if (ability)
				ability->update(dt);
		}
	}

}
