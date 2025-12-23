#include "FireballAbility.h"
#include "Projectile.h"

namespace Nawia::Entity {

	FireballAbility::FireballAbility(const std::shared_ptr<Texture2D>& projectile_tex)
		: Ability("Fireball", Entity::getAbilityStatsFromJson("Fireball"), AbilityTargetType::UNIT), _texture(projectile_tex) {}

	std::unique_ptr<Entity> FireballAbility::cast(Entity* caster, float target_x, float target_y) 
	{
		if (!isReady())
			return nullptr;

		startCooldown();

		return std::make_unique<Projectile>(caster->getX(), caster->getY(), target_x, target_y, _texture, _stats);
	}

} // namespace Nawia::Entity
