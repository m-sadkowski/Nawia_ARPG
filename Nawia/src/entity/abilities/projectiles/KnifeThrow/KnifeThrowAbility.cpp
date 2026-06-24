#include "KnifeThrowAbility.h"

namespace Nawia::Entity {

	KnifeThrowAbility::KnifeThrowAbility(const std::string& model_path,
										 const float model_scale,
										 const std::shared_ptr<Texture2D>& hit_tex,
										 const std::shared_ptr<Texture2D>& icon_tex,
										 const float facing_offset,
										 Core::ResourceManager* resource_manager)
		: ProjectileAbility(
			  "Knife Throw",
			  "KnifeThrow",
			  AbilityTargetType::UNIT,
			  "Knife Projectile",
			  model_path,
			  model_scale,
			  hit_tex,
			  icon_tex,
			  facing_offset,
			  resource_manager) {}

} // namespace Nawia::Entity
