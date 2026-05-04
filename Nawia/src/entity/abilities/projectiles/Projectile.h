#pragma once

#include <AbilityEffect.h>
#include <AbilityStats.h>

#include <string>

namespace Nawia::Entity {

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
		 */
		Projectile(const std::string& name, float x, float y, float target_x, float target_y,
			const std::string& model_path, float model_scale,
			const AbilityStats& stats, Entity* caster,
			const std::shared_ptr<Texture2D>& hit_tex = nullptr,
			float facing_offset = 0.0f);

		void update(float dt) override;

		[[nodiscard]] bool checkCollision(const std::shared_ptr<Entity>& target) const override;
		void onCollision(const std::shared_ptr<Entity>& target) override;

		/** @brief Ustawia lot nad ziemią zamiast na poziomie Y=0. */
		[[nodiscard]] Vector3 getWorldPos3D() const override { return { _pos.x, _fly_height, _pos.y }; }

	private:
		float _speed;
		float _vel_x, _vel_y;
		float _fly_height = 1.0f;  ///< Wysokość lotu nad ziemią, mniej więcej poziom tułowia.
		std::shared_ptr<Texture2D> _hit_texture;
		Entity* _caster;
	};

} // namespace Nawia::Entity
