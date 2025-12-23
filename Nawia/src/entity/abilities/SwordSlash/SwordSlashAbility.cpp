#include "SwordSlashAbility.h"
#include "SwordSlashEffect.h"

#include <MathUtils.h>

#include <iostream>

namespace Nawia::Entity {

	SwordSlashAbility::SwordSlashAbility(const std::shared_ptr<Texture2D>& slash_tex)
	    : Ability("Sword Slash", Entity::getAbilityStatsFromJson("Sword Slash"), AbilityTargetType::POINT), _slash_tex(slash_tex) {}

	std::unique_ptr<Entity> SwordSlashAbility::cast(Entity* caster, const float target_x, const float target_y) 
	{
	  startCooldown();

	  // calculate angle
	  const float dx = target_x - caster->getX();
	  const float dy = target_y - caster->getY();
	  const float angle = static_cast<float>(std::atan2(dy, dx) * 180.0f / Core::pi);

	  const float length = std::sqrt(dx * dx + dy * dy);
	  const float spawn_x = caster->getX() + (dx / length);
	  const float spawn_y =  caster->getY() + (dy / length);

	  return std::make_unique<SwordSlashEffect>(spawn_x, spawn_y, angle, _slash_tex, _stats);
	}

} // namespace Nawia::Entity
