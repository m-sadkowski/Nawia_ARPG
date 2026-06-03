#pragma once

#include <AbilityEffect.h>

namespace Nawia::Entity {

	/**
	 * @brief Tymczasowy obszar trafienia dla bezbronnych ciosow.
	 *
	 * Efekt istnieje jak encja ability i jest sprawdzany przez system kolizji.
	 * Obsluguje dwa ksztalty: stozek przed graczem oraz prostokat przydatny dla
	 * bardziej "liniowych" atakow.
	 */
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
		/** @brief Sprawdza, czy dany przeciwnik lezy w wybranym ksztalcie obszaru trafienia. */
		[[nodiscard]] bool checkCollision(const std::shared_ptr<Entity>& target) const override;
		/** @brief Zadaje obrazenia, zapamietuje agresora i opcjonalnie odpycha cel. */
		void onCollision(const std::shared_ptr<Entity>& target) override;

	private:
		/** @brief Nieposiadany wskaznik do castera, zyjacego dluzej niz krotki efekt. */
		Entity* _caster = nullptr;
		Shape _shape = Shape::Cone;
		/** @brief Szerokosc stozka w stopniach albo szerokosc prostokata w jednostkach swiata. */
		float _width = 1.0f;
		float _knockback_distance = 0.0f;
	};

} // namespace Nawia::Entity
