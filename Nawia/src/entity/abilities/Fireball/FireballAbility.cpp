#include "FireballAbility.h"
#include "Projectile.h"

namespace Nawia::Entity 
{

	FireballAbility::FireballAbility(const std::shared_ptr<SDL_Texture> &projectile_tex) 
		: Ability("Fireball", 1.0f, 10.0f, AbilityTargetType::UNIT), _texture(projectile_tex) {}

	std::unique_ptr<Entity> FireballAbility::cast(Entity *caster, float target_x, float target_y)
	{
	  if (!isReady())
	    return nullptr;

	  startCooldown();

	  constexpr float speed = 10.0f;
	  constexpr int damage = 20;
	  float duration = _cast_range / speed;

	  return std::make_unique<Projectile>(caster->getX(), caster->getY(), target_x, target_y, speed, _texture, damage, duration);
	}

} // namespace Nawia::Entity
