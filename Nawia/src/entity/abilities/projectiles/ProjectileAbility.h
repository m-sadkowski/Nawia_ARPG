#pragma once

#include <Ability.h>

#include <raylib.h>

#include <memory>
#include <string>

namespace Nawia::Entity {

	/**
	 * @class ProjectileAbility
	 * @brief Wspólna baza umiejętności tworzących pocisk 3D.
	 */
	class ProjectileAbility : public Ability {
	public:
		/**
		 * @brief Tworzy umiejętność pociskową z modelem, skalą i opcjonalnym efektem trafienia.
		 */
		ProjectileAbility(std::string ability_name,
						  const std::string& stats_key,
						  AbilityTargetType target_type,
						  std::string projectile_name,
						  std::string model_path,
						  float model_scale,
						  const std::shared_ptr<Texture2D>& hit_texture,
						  const std::shared_ptr<Texture2D>& icon_texture,
						  float facing_offset = 0.0f);

		/**
		 * @brief Tworzy pocisk lecący w stronę wskazanego punktu.
		 */
		AbilitySpawn cast(float target_x, float target_y) override;

	protected:
		/**
		 * @brief Zwraca punkt startu pocisku.
		 */
		[[nodiscard]] virtual Vector2 getSpawnPosition() const;

    /** @brief Zwraca logiczną pozycję źródła użycia. */
		[[nodiscard]] Vector2 getCasterPosition() const;

    /** @brief Zwraca środek źródła użycia. */
		[[nodiscard]] Vector2 getCasterCenter() const;

	private:
		std::string _projectile_name;
		std::string _model_path;
		float _model_scale = 1.0f;
		std::shared_ptr<Texture2D> _hit_texture;
		float _facing_offset = 0.0f;
	};

} // namespace Nawia::Entity
