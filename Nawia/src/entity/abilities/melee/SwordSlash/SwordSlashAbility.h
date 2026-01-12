#pragma once
#include "Ability.h"

#include <raylib.h>
#include <memory>

namespace Nawia::Entity {

	class SwordSlashAbility : public Ability {
	public:
		SwordSlashAbility(const std::shared_ptr<Texture2D>& slash_tex, const std::shared_ptr<Texture2D>& icon_tex);

		void update(float dt) override;
		std::unique_ptr<Entity> cast(float target_x, float target_y) override;

	private:
		std::shared_ptr<Texture2D> _slash_tex;
		bool _is_active = false;
		float _target_x = 0.0f;
		float _target_y = 0.0f;
		
		bool _has_spawned = false;
		float _active_time = 0.0f;
		float _spawn_delay = 0.0f;
	};

} // namespace Nawia::Entity
