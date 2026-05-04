#pragma once

#include <AbilityEffect.h>
#include <AbilityStats.h>

#include <memory>
#include <raylib.h>

namespace Nawia::Entity {

	/**
	 * @class SwordSlashEffect
	 * @brief Krótkotrwały stożek obrażeń tworzony przez cięcie mieczem.
	 */
	class SwordSlashEffect : public AbilityEffect {
	public:
		/**
		 * @brief Tworzy efekt cięcia w punkcie świata i wiąże go ze źródłem użycia.
		 */
		SwordSlashEffect(float x,
						 float y,
						 float angle,
						 const std::shared_ptr<Texture2D>& tex,
						 const AbilityStats& stats,
						 Entity* caster);

		/** @brief Aktualizuje czas życia efektu. */
		void update(float dt) override;

		/** @brief Rysuje diagnostyczny wolumen cięcia. */
		void render(const Camera3D& camera) override;

		/** @brief Zwraca kąt cięcia w stopniach. */
		[[nodiscard]] float getAngle() const { return _angle; }

		/** @brief Sprawdza, czy cięcie trafia wskazaną encję. */
		[[nodiscard]] bool checkCollision(const std::shared_ptr<Entity>& target) const override;

		/** @brief Nakłada obrażenia po trafieniu celu. */
		void onCollision(const std::shared_ptr<Entity>& target) override;

	private:
		float _angle = 0.0f;
		Entity* _caster = nullptr;
	};

} // namespace Nawia::Entity
