#pragma once

#include <AbilityEffect.h>
#include <AbilityStats.h>

#include <memory>
#include <string>

namespace Nawia::Entity {

	class CobwebProjectile : public AbilityEffect {
	public:
		CobwebProjectile(
			float x,
			float y,
			float target_x,
			float target_y,
			float start_height,
			float target_height,
			const Model* shared_model,
			Entity* caster);

		void update(float dt) override;
		void render(const Camera3D& camera) override;
		[[nodiscard]] bool checkCollision(const std::shared_ptr<Entity>& target) const override;
		void onCollision(const std::shared_ptr<Entity>& target) override;
		[[nodiscard]] Vector3 getWorldPos3D() const override { return {_pos.x, _flight_height, _pos.y}; }

	private:
		void configureMovement(float target_x, float target_y);
		void updateAttachedPosition();

		float _vel_x = 0.0f;
		float _vel_y = 0.0f;
		float _speed = 9.0f;
		float _start_x = 0.0f;
		float _start_y = 0.0f;
		float _travel_distance = 0.0f;
		float _start_height = 1.0f;
		float _target_height = 1.0f;
		float _flight_height = 1.0f;
		float _attached_timer = 0.0f;
		std::weak_ptr<Entity> _attached_target;
		Entity* _caster = nullptr;
	};

} // namespace Nawia::Entity
