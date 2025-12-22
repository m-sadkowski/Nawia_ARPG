#pragma once
#include "AbilityEffect.h"

#include <vector>

namespace Nawia::Entity {

	class SwordSlashEffect : public AbilityEffect {
	public:
		SwordSlashEffect(float x, float y, float angle, const std::shared_ptr<Texture2D>& tex, int damage);

		void update(float dt) override;
		void render(float camera_x, float camera_y) override;

		[[nodiscard]] float getAngle() const { return _angle; }
		[[nodiscard]] bool checkCollision(const std::shared_ptr<Entity>& target) const override;
		void onCollision(const std::shared_ptr<Entity>& target) override;

	private:
		float _angle;
	};

} // namespace Nawia::Entity
