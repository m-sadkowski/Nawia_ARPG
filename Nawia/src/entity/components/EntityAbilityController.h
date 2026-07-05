#pragma once

#include <memory>
#include <vector>

namespace Nawia::Entity {
	class Ability;
	class Entity;

	class EntityAbilityController {
	public:
		void addAbility(Entity& owner, const std::shared_ptr<Ability>& ability);
		void setAbility(Entity& owner, int index, const std::shared_ptr<Ability>& ability);
		[[nodiscard]] std::shared_ptr<Ability> getAbility(int index) const;
		[[nodiscard]] const std::vector<std::shared_ptr<Ability>>& getAbilities() const { return _abilities; }
		void updateAbilities(float dt) const;

	private:
		std::vector<std::shared_ptr<Ability>> _abilities;
	};

}
