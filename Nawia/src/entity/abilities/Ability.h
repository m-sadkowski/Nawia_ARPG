#pragma once
#include "AbilityEffect.h"
#include "AbilityStats.h"

#include <functional>
#include <vector>
#include <string>
#include <memory>

namespace Nawia::Entity
{
	class Entity;
	
	enum class AbilityTargetType {
		POINT, // casted in location
		UNIT,  // casted on enemies
		SELF   // casted on player
	};

	class Ability {
	public:
		Ability(std::string name, const AbilityStats& stats, AbilityTargetType target_type);

		virtual ~Ability() = default;

		void update(float dt);

		[[nodiscard]] bool isReady() const;
		virtual std::unique_ptr<Entity> cast(float target_x, float target_y) = 0;
		[[nodiscard]] std::string getName() const;
		[[nodiscard]] float getCastRange() const;
		[[nodiscard]] AbilityTargetType getTargetType() const;
		[[nodiscard]] const AbilityStats& getStats() const { return _stats; }
		
		void setCaster(Entity* caster) { _caster = caster; }
		[[nodiscard]] Entity* getCaster() const { return _caster; }

	protected:
		std::string _name;
		AbilityStats _stats;
		float _cooldown_timer;
		AbilityTargetType _target_type;
		Entity* _caster;

		void startCooldown() { _cooldown_timer = _stats.cooldown; }
	};

} // namespace Nawia::Entity