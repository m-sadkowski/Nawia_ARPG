#pragma once

#include <AbilityEffect.h>
#include <AbilityStats.h>

#include <raylib.h>

#include <memory>
#include <string>

namespace Nawia::Entity {

	/**
	 * @class Projectile
	 * @brief Efekt umiejetnosci poruszajacy sie jako pocisk 3D.
	 */
	class Projectile : public AbilityEffect {
	public:
		/**
		 * @brief Tworzy pocisk oparty o model 3D.
		 */
		Projectile(const std::string& name,
				   float x,
				   float y,
				   float target_x,
				   float target_y,
				   const std::string& model_path,
				   float model_scale,
				   const AbilityStats& stats,
				   Entity* caster,
				   float target_height,
				   const std::shared_ptr<Texture2D>& hit_tex = nullptr,
				   float facing_offset = 0.0f,
				   const Model* shared_model = nullptr);

		/** @brief Aktualizuje pozycje pocisku i czas zycia. */
		void update(float dt) override;

		/** @brief Sprawdza, czy pocisk trafia wskazana encje. */
		[[nodiscard]] bool checkCollision(const std::shared_ptr<Entity>& target) const override;

		/** @brief Naklada obrazenia, tworzy efekt trafienia i konczy zycie pocisku. */
		void onCollision(const std::shared_ptr<Entity>& target) override;

		/** @brief Zwraca pozycje 3D na aktualnej wysokosci lotu. */
		[[nodiscard]] Vector3 getWorldPos3D() const override { return {_pos.x, _flight_height, _pos.y}; }

	private:
		/** @brief Ustawia kierunek lotu i rotacje modelu. */
		void configureMovement(float target_x, float target_y, float facing_offset);

		float _speed = 0.0f;
		float _vel_x = 0.0f;
		float _vel_y = 0.0f;
		float _previous_x = 0.0f;
		float _previous_y = 0.0f;
		float _start_x = 0.0f;
		float _start_y = 0.0f;
		float _travel_distance = 0.0f;
		float _start_height = 1.0f;
		float _target_height = 1.0f;
		float _flight_height = 1.0f;
		std::shared_ptr<Texture2D> _hit_texture;
		Entity* _caster = nullptr;
	};

} // namespace Nawia::Entity
