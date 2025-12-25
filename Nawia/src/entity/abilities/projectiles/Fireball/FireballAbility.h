#pragma once
#include "Ability.h"

#include <raylib.h>
#include <memory>

namespace Nawia::Entity {

	class FireballAbility : public Ability {
	public:
		FireballAbility(const std::shared_ptr<Texture2D>& projectile_tex, const std::shared_ptr<Texture2D>& hit_tex);

		std::unique_ptr<Entity> cast(float target_x, float target_y) override;

	private:
		std::shared_ptr<Texture2D> _texture;
		std::shared_ptr<Texture2D> _hit_texture;
	};

} // namespace Nawia::Entity
