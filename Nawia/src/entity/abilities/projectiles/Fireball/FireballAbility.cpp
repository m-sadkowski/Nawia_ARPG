#include "FireballAbility.h"
#include <Projectile.h>

namespace Nawia::Entity {

	FireballAbility::FireballAbility(const std::string& model_path, const float model_scale, const std::shared_ptr<Texture2D>& hit_tex, const std::shared_ptr<Texture2D>& icon_tex)
		: Ability("Fireball", Entity::getAbilityStatsFromJson("Fireball"), AbilityTargetType::UNIT, icon_tex), 
		  _model_path(model_path), _model_scale(model_scale), _hit_texture(hit_tex) {}

	std::unique_ptr<Entity> FireballAbility::cast(const float target_x, const float target_y) 
	{
		if (!isReady())
			return nullptr;

		startCooldown();

		return std::make_unique<Projectile>("Fireball Projectile", _caster->getX(), _caster->getY(), target_x, target_y, _model_path, _model_scale, _stats, _caster, _hit_texture);
	}

} // namespace Nawia::Entity
