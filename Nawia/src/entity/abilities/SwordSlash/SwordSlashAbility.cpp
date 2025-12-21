#include "SwordSlashAbility.h"
#include "SwordSlashEffect.h"
#include <iostream>
#include <MathUtils.h>

namespace Nawia::Entity
{

	SwordSlashAbility::SwordSlashAbility(const std::shared_ptr<SDL_Texture> &slash_tex) 
		: Ability("Sword Slash", 1.0f, 2.0f, AbilityTargetType::POINT), _slash_tex(slash_tex) {}

	std::unique_ptr<Entity> SwordSlashAbility::cast(Entity* caster, const float target_x, const float target_y) {
	  startCooldown();

	  // calculate angle
	  const float dx = target_x - caster->getX();
	  const float dy = target_y - caster->getY();
	  float angle = static_cast<float>(std::atan2(dy, dx) * 180.0f / Core::pi);

	  // offset spawn in front of player
	  const float spawn_x = caster->getX() + (dx / std::sqrt(dx * dx + dy * dy)) * 0.5f;
	  const float spawn_y = caster->getY() + (dy / std::sqrt(dx * dx + dy * dy)) * 0.5f;

	  return std::make_unique<SwordSlashEffect>(spawn_x, spawn_y, angle, _slash_tex, 20);
	}

} // namespace Nawia::Entity
