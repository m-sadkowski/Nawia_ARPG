#pragma once

#include <AbilityEffect.h>

namespace Nawia::Entity {

	class UnarmedMeleeEffect : public AbilityEffect {
	public:
		enum class Shape {
			Cone,
			ForwardRectangle
		};

		UnarmedMeleeEffect(
			float x,
			float y,
			float angle,
			const AbilityStats& stats,
			Entity* caster,
			Shape shape,
			float width,
			float knockback_distance);

		void render(const Camera3D& camera) override;
		[[nodiscard]] bool checkCollision(const std::shared_ptr<Entity>& target) const override;
		void onCollision(const std::shared_ptr<Entity>& target) override;

	private:
		Entity* _caster = nullptr;
		Shape _shape = Shape::Cone;
		float _width = 1.0f;
		float _knockback_distance = 0.0f;
	};

} // namespace Nawia::Entity
