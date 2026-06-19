#pragma once

#include <Ability.h>
#include <UnarmedMeleeEffect.h>

#include <memory>
#include <string>

namespace Nawia::Entity {

	/**
	 * @brief Konfigurowalna umiejetnosc walki w zwarciu dla gracza i prostych ciosow.
	 *
	 * Ability moze zadac obrazenia bezposrednio aktualnemu celowi albo po opoznieniu
	 * stworzyc `UnarmedMeleeEffect`, ktory wykryje kolizje w ksztalcie stozka lub
	 * prostokata. Dane liczbowe pochodza z `abilities.json`.
	 */
	class UnarmedMeleeAbility : public Ability {
	public:
		UnarmedMeleeAbility(
			std::string ability_name,
			const std::string& stats_key,
			std::string animation_name,
			AbilityTargetType target_type,
			bool direct_target_hit,
			UnarmedMeleeEffect::Shape effect_shape,
			float spawn_ratio,
			float hitbox_width,
			float knockback_distance,
			bool ping_pong_animation,
			const std::shared_ptr<Texture2D>& icon_tex);

		void update(float dt) override;
		void cancel() override;
		/** @brief Rozpoczyna animacje ataku i ustawia moment zadania obrazen/efektu. */
		AbilitySpawn cast(float target_x, float target_y) override;

	private:
		/** @brief Szacuje czas animacji z liczby klatek i predkosci animacji castera. */
		[[nodiscard]] float calculateAnimationDuration() const;
		/** @brief Tworzy obszar trafienia w odpowiednim momencie animacji. */
		void spawnEffect();

		std::string _animation_name;
		/** @brief Wariant bez efektu przestrzennego, trafiajacy tylko aktualny cel. */
		bool _direct_target_hit = false;
		UnarmedMeleeEffect::Shape _effect_shape = UnarmedMeleeEffect::Shape::Cone;
		/** @brief Procent czasu animacji, po ktorym pojawia sie obszar trafienia. */
		float _spawn_ratio = 0.45f;
		float _hitbox_width = 55.0f;
		float _knockback_distance = 0.0f;
		bool _ping_pong_animation = true;
		/** @brief Stan aktywnego uzycia oczekujacego na utworzenie obszaru trafienia. */
		bool _is_active = false;
		bool _has_spawned = false;
		float _active_time = 0.0f;
		float _spawn_delay = 0.0f;
	};

} // namespace Nawia::Entity
