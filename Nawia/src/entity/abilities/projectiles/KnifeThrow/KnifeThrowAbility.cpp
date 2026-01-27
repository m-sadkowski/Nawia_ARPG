#include "KnifeThrowAbility.h"

#include "Collider.h"
#include "Projectile.h"

namespace Nawia::Entity {

	KnifeThrowAbility::KnifeThrowAbility(const std::shared_ptr<Texture2D>& projectile_tex, const std::shared_ptr<Texture2D>& hit_tex, const std::shared_ptr<Texture2D>& icon_tex)
		: Ability("Knife Throw", Entity::getAbilityStatsFromJson("KnifeThrow"), AbilityTargetType::UNIT, icon_tex), _texture(projectile_tex), _hit_texture(hit_tex) {}

	std::unique_ptr<Entity> KnifeThrowAbility::cast(const float target_x, const float target_y) 
	{
		if (!isReady())
			return nullptr;

		startCooldown();

		return std::make_unique<Projectile>("Knife Projectile", _caster->getCollider()->getPosition().x, _caster->getCollider()->getPosition().y, target_x, target_y, _texture, _hit_texture, _stats, _caster);
	}

} // namespace Nawia::Entity
