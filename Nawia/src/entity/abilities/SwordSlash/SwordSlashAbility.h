#pragma once
#include "Ability.h"

#include <raylib.h>
#include <memory>

namespace Nawia::Entity {

	class SwordSlashAbility : public Ability {
	public:
		SwordSlashAbility(const std::shared_ptr<Texture2D>& slash_tex);

		std::unique_ptr<Entity> cast(Entity* caster, float target_x, float target_y) override;

	private:
		std::shared_ptr<Texture2D> _slash_tex;
	};

} // namespace Nawia::Entity
