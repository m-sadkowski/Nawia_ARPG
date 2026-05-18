#include "FireballAbility.h"

namespace Nawia::Entity {

	FireballAbility::FireballAbility(const std::string& model_path,
									 const float model_scale,
									 const std::shared_ptr<Texture2D>& hit_tex,
									 const std::shared_ptr<Texture2D>& icon_tex,
									 Core::ResourceManager* resource_manager)
		: ProjectileAbility(
			  "Fireball",
			  "Fireball",
			  AbilityTargetType::UNIT,
			  "Fireball Projectile",
			  model_path,
			  model_scale,
			  hit_tex,
			  icon_tex,
			  0.0f,
			  resource_manager) {}

	Vector2 FireballAbility::getSpawnPosition() const {
		return getCasterPosition();
	}

} // namespace Nawia::Entity
