#pragma once
#include "Entity.h"
#include "AbilityEffect.h"
#include <functional>
#include <vector>
#include <string>


namespace Nawia::Entity
{

	enum class AbilityTargetType {
		POINT, // casted in location
		UNIT,  // casted on enemies
		SELF   // casted on player
	};

	class Ability {
	public:
		Ability(std::string name, float cooldown, float cast_range, AbilityTargetType target_type);

		virtual ~Ability() = default;

		void update(float dt);

		[[nodiscard]] bool isReady() const;
		virtual std::unique_ptr<Entity> cast(Entity* caster, float target_x, float target_y) = 0;
		[[nodiscard]] std::string getName() const;
		[[nodiscard]] float getCastRange() const;
		[[nodiscard]] AbilityTargetType getTargetType() const;

	protected:
		std::string _name;
		float _cooldown_max;
		float _cooldown_timer;
		float _cast_range;
		AbilityTargetType _target_type;

		void startCooldown() { _cooldown_timer = _cooldown_max; }
	};

} // namespace Nawia::Entity