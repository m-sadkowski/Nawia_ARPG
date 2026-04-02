#pragma once

#include "Ability.h"

#include <raylib.h>
#include <memory>
#include <string>

namespace Nawia::Entity {

	/**
	 * @class KnifeThrowAbility
	 * @brief Ranged knife throwing ability for Bandit enemies.
	 * 
	 * Creates a 3D knife projectile that travels toward the target.
	 */
	class KnifeThrowAbility : public Ability {
	public:
		KnifeThrowAbility(const std::string& model_path, float model_scale, const std::shared_ptr<Texture2D>& hit_tex, const std::shared_ptr<Texture2D>& icon_tex, float facing_offset = 0.0f);

		std::unique_ptr<Entity> cast(float target_x, float target_y) override;

	private:
		std::string _model_path;
		float _model_scale;
		std::shared_ptr<Texture2D> _hit_texture;
		float _facing_offset;
	};

} // namespace Nawia::Entity
