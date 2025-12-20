#pragma once
#include <Ability.h>

namespace Nawia::Entity 
{

	class SwordSlashAbility : public Ability {
	public:
	  SwordSlashAbility(const std::shared_ptr<SDL_Texture> &slash_tex);

	  std::unique_ptr<Entity> cast(Entity *caster, float target_x, float target_y) override;

	private:
	  std::shared_ptr<SDL_Texture> _slash_tex;
	};

} // namespace Nawia::Entity
