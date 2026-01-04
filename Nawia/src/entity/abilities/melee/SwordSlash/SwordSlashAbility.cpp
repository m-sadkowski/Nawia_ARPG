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
		// Force rotation to target immediately
		_caster->rotateTowardsCenter(target_x, target_y);

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

				const Vector2 caster_center = _caster->getCenter();

				// This ensures that if the player rotated during the cast time (via PlayerController aiming), 
				// the slash goes where the player is looking, not where the mouse WAS.
				const float angle = _caster->getRotation();
				const float spawn_x = caster_center.x;
				const float spawn_y = caster_center.y;

				const auto slash = std::make_shared<SwordSlashEffect>(spawn_x, spawn_y, -angle, _slash_tex, _stats);
				_caster->addPendingSpawn(slash);
			}
		}
	}

} // namespace Nawia::Entity
