#pragma once

#include <AbilityEffect.h>

#include <memory>
#include <raylib.h>

namespace Nawia::Entity {

	/**
	 * @class ProjectileHitEffect
	 * @brief Krótki efekt wizualny odpalany w miejscu trafienia pocisku.
	 */
	class ProjectileHitEffect : public AbilityEffect {
	public:
		/**
		 * @brief Tworzy animowany efekt trafienia w podanym punkcie świata.
		 */
		ProjectileHitEffect(float x, float y, const std::shared_ptr<Texture2D>& tex);

		/** @brief Aktualizuje klatkę animacji i czas życia efektu. */
		void update(float dt) override;

		/** @brief Renderuje aktualną klatkę efektu trafienia. */
		void render(const Camera3D& camera) override;

		/** @brief Efekt wizualny nie koliduje z innymi encjami. */
		[[nodiscard]] bool checkCollision(const std::shared_ptr<Entity>& target) const override { return false; }

	private:
		int _frame_width = 0;
		int _frame_height = 0;
		int _current_frame = 0;
		float _frame_timer = 0.0f;
		int _total_frames = 1;
		float _frame_duration = 0.5f;
	};

} // namespace Nawia::Entity
