#pragma once

#include "Ability.h"

#include <raylib.h>
#include <memory>
#include <string>

namespace Nawia::Entity {

	class FireballAbility : public Ability {
	public:
		FireballAbility(const std::string& model_path, float model_scale, const std::shared_ptr<Texture2D>& hit_tex, const std::shared_ptr<Texture2D>& icon_tex);

		std::unique_ptr<Entity> cast(float target_x, float target_y) override;

	private:
		std::string _model_path;
		float _model_scale;
		std::shared_ptr<Texture2D> _hit_texture;
	};

} // namespace Nawia::Entity
