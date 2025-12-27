#include "SwordSlashAbility.h"
#include "SwordSlashEffect.h"
#include "Entity.h"

#include <MathUtils.h>

#include <iostream>

namespace Nawia::Entity {

	SwordSlashAbility::SwordSlashAbility(const std::shared_ptr<Texture2D>& slash_tex)
	    : Ability("Sword Slash", Entity::getAbilityStatsFromJson("Sword Slash"), AbilityTargetType::POINT), _slash_tex(slash_tex) {}

	std::unique_ptr<Entity> SwordSlashAbility::cast(const float target_x, const float target_y) 
	{
		startCooldown();
		_caster->playAnimation("attack", false, true);

		// store target data and activate delayed spawn
		_is_active = true;
		_target_x = target_x;
		_target_y = target_y;

		// return nullptr because we aren't spawning the effect YET
		return nullptr;
	}

	void SwordSlashAbility::update(const float dt)
	{
		Ability::update(dt);

		if (_is_active)
		{
			// wait for animation to finish (unlock)
			if (!_caster->isAnimationLocked())
			{
				_is_active = false;

				// START: spawning Logic from previous cast()
				const float dx = _target_x - _caster->getX();
				const float dy = _target_y - _caster->getY();
				const float angle = static_cast<float>(std::atan2(dy, dx) * 180.0f / PI);

				float length = std::sqrt(dx * dx + dy * dy);
				if (length == 0.0f) length = 1.0f; // prevent div by zero

				const float spawn_x = _caster->getX() + (dx / length);
				const float spawn_y = _caster->getY() + (dy / length); // spawn offset by 1 unit

				auto slash = std::make_shared<SwordSlashEffect>(spawn_x, spawn_y, angle, _slash_tex, _stats);
				_caster->addPendingSpawn(slash);
				// END: spawning Logic
			}
		}
	}

} // namespace Nawia::Entity
