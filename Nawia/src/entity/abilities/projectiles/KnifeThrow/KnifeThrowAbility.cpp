#include "KnifeThrowAbility.h"
#include <Projectile.h>

namespace Nawia::Entity {

	KnifeThrowAbility::KnifeThrowAbility(const std::string& model_path, const float model_scale, const std::shared_ptr<Texture2D>& hit_tex, const std::shared_ptr<Texture2D>& icon_tex, const float facing_offset)
		: Ability("Knife Throw", Entity::getAbilityStatsFromJson("KnifeThrow"), AbilityTargetType::UNIT, icon_tex), 
		  _model_path(model_path), _model_scale(model_scale), _hit_texture(hit_tex), _facing_offset(facing_offset) {}

	std::unique_ptr<Entity> KnifeThrowAbility::cast(const float target_x, const float target_y) 
	{
		if (!isReady())
			return nullptr;

		startCooldown();

		return std::make_unique<Projectile>("Knife Projectile", _caster->getCenter().x, _caster->getCenter().y, target_x, target_y, _model_path, _model_scale, _stats, _caster, _hit_texture, _facing_offset);
	}

} // namespace Nawia::Entity
