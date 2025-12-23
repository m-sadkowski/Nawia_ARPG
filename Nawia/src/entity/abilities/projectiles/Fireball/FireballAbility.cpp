#include "FireballAbility.h"
#include "Projectile.h"

namespace Nawia::Entity {

	FireballAbility::FireballAbility(const std::shared_ptr<Texture2D>& projectile_tex)
		: Ability("Fireball", Entity::getAbilityStatsFromJson("Fireball"), AbilityTargetType::UNIT), _texture(projectile_tex) {}

	std::unique_ptr<Entity> FireballAbility::cast(const float target_x, const float target_y) 
	{
		if (!isReady())
			return nullptr;

		startCooldown();

		return std::make_unique<Projectile>(_caster->getX(), _caster->getY(), target_x, target_y, _texture, _stats, _caster);
	}

} // namespace Nawia::Entity
