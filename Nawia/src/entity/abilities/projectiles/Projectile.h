#pragma once

#include "AbilityEffect.h"
#include "AbilityStats.h"

#include <string>

namespace Nawia::Entity {

	class Projectile : public AbilityEffect {
	public:
		/**
		 * @brief Construct a 3D model-based projectile.
		 * @param name Display name
		 * @param x Start X (world)
		 * @param y Start Y (maps to world Z)
		 * @param target_x Target X
		 * @param target_y Target Y (world Z)
		 * @param model_path Path to the .glb model file
		 * @param model_scale Scale for the projectile model
		 * @param stats Ability stats (speed, damage, hitbox_radius, etc.)
		 * @param caster The entity that fired this projectile
		 * @param hit_tex Optional texture for hit effect
		 */
		Projectile(const std::string& name, float x, float y, float target_x, float target_y,
			const std::string& model_path, float model_scale,
			const AbilityStats& stats, Entity* caster,
			const std::shared_ptr<Texture2D>& hit_tex = nullptr,
			float facing_offset = 0.0f);

		void update(float dt) override;

		[[nodiscard]] bool checkCollision(const std::shared_ptr<Entity>& target) const override;
		void onCollision(const std::shared_ptr<Entity>& target) override;

		/** @brief Override to fly at a height above ground instead of Y=0. */
		[[nodiscard]] Vector3 getWorldPos3D() const override { return { _pos.x, _fly_height, _pos.y }; }

	private:
		float _speed;
		float _vel_x, _vel_y;
		float _fly_height = 1.0f;  ///< Y height above ground (torso level)
		std::shared_ptr<Texture2D> _hit_texture;
		Entity* _caster;
	};

} // namespace Nawia::Entity