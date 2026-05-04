#pragma once

#include <AbilityEffect.h>
#include <AbilityStats.h>

#include <raylib.h>

#include <memory>
#include <string>

namespace Nawia::Entity {

	/**
	 * @class Projectile
	 * @brief Efekt umiejętności poruszający się jako pocisk 3D.
	 */
	class Projectile : public AbilityEffect {
	public:
		/**
		 * @brief Tworzy pocisk oparty o model 3D.
		 * @param name Nazwa pocisku.
		 * @param x Startowa pozycja X.
		 * @param y Startowa pozycja Y, mapowana na Z świata.
		 * @param target_x Pozycja X celu.
		 * @param target_y Pozycja Y celu, mapowana na Z świata.
		 * @param model_path Ścieżka do modelu `.glb`.
		 * @param model_scale Skala modelu pocisku.
		 * @param stats Statystyki prędkości, obrażeń i hitboxa.
		 * @param caster Encja, która wystrzeliła pocisk.
		 * @param hit_tex Opcjonalna tekstura efektu trafienia.
		 * @param facing_offset Offset wizualnego kierunku modelu.
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
				   const std::shared_ptr<Texture2D>& hit_tex = nullptr,
				   float facing_offset = 0.0f);

		/** @brief Aktualizuje pozycję pocisku i czas życia. */
		void update(float dt) override;

		/** @brief Sprawdza, czy pocisk trafia wskazaną encję. */
		[[nodiscard]] bool checkCollision(const std::shared_ptr<Entity>& target) const override;

		/** @brief Nakłada obrażenia, tworzy efekt trafienia i kończy życie pocisku. */
		void onCollision(const std::shared_ptr<Entity>& target) override;

		/** @brief Zwraca pozycję 3D z lotem nad ziemią. */
		[[nodiscard]] Vector3 getWorldPos3D() const override { return {_pos.x, _fly_height, _pos.y}; }

	private:
		/** @brief Ustawia kierunek lotu i rotację modelu. */
		void configureMovement(float target_x, float target_y, float facing_offset);

		float _speed = 0.0f;
		float _vel_x = 0.0f;
		float _vel_y = 0.0f;
		float _fly_height = 1.0f; ///< Wysokość lotu nad ziemią, mniej więcej poziom tułowia.
		std::shared_ptr<Texture2D> _hit_texture;
		Entity* _caster = nullptr;
	};

} // namespace Nawia::Entity
