#include "SwordSlashAbility.h"
#include "SwordSlashEffect.h"
#include "Entity.h"

#include <MathUtils.h>

#include <iostream>

namespace Nawia::Entity {

	SwordSlashAbility::SwordSlashAbility(const std::shared_ptr<Texture2D>& slash_tex, const std::shared_ptr<Texture2D>& icon_tex)
	    : Ability("Sword Slash", Entity::getAbilityStatsFromJson("Sword Slash"), AbilityTargetType::POINT, icon_tex), _slash_tex(slash_tex) {}

	std::unique_ptr<Entity> SwordSlashAbility::cast(const float target_x, const float target_y) 
	{
		if (!isReady()) return nullptr;

		// Force rotation to target immediately
		_caster->rotateTowardsCenter(target_x, target_y);

		startCooldown();
		_caster->playAnimation("attack", false, true);

		// store target data and activate delayed spawn
		_is_active = true;
		_has_spawned = false;
		_active_time = 0.0f;
		_target_x = target_x;
		_target_y = target_y;

		// Calculate dynamic spawn delay based on animation duration
		// Assuming 60 FPS update rate for animations as per Entity::updateAnimation
		const int frames = _caster->getAnimationFrameCount("attack");
		const float duration = (frames > 0) ? (frames / 60.0f) : 1.0f; 
		
		// Trigger hit at 60% of the animation (impact point)
		_spawn_delay = duration * 0.6f;

		// return nullptr because we aren't spawning the effect YET
		return nullptr;
	}

	void SwordSlashAbility::update(const float dt)
	{
		Ability::update(dt);

		if (_is_active)
		{
			_active_time += dt;

			// Spawn point: Calculated dynamic delay OR when animation ends (fallback)
			bool should_spawn = !_has_spawned && (_active_time >= _spawn_delay || !_caster->isAnimationLocked());

			if (should_spawn)
			{
				_has_spawned = true;

				const Vector2 caster_center = _caster->getCenter();

				// This ensures that if the player rotated during the cast time (via PlayerController aiming), 
				// the slash goes where the player is looking, not where the mouse WAS.
				const float angle = _caster->getRotation();
				const float spawn_x = caster_center.x;
				const float spawn_y = caster_center.y;

				const auto slash = std::make_shared<SwordSlashEffect>(spawn_x, spawn_y, -angle, _slash_tex, _stats, _caster);
				_caster->addPendingSpawn(slash);
			}

			// End ability state when animation is unlocked
			if (!_caster->isAnimationLocked())
			{
				_is_active = false;
			}
		}
	}

} // namespace Nawia::Entity
