#pragma once

#include <Ability.h>

#include <memory>
#include <raylib.h>
#include <string>

namespace Nawia::Entity {

	/**
	 * @class FireballAbility
	 * @brief Dystansowa umiejętność tworząca pocisk ognistej kuli.
	 */
	class FireballAbility : public Ability {
	public:
		/**
		 * @brief Tworzy umiejętność z modelem pocisku, skalą i teksturami UI/trafienia.
		 */
		FireballAbility(const std::string& model_path,
						float model_scale,
						const std::shared_ptr<Texture2D>& hit_tex,
						const std::shared_ptr<Texture2D>& icon_tex);

		/**
		 * @brief Wystrzeliwuje pocisk w stronę wskazanego punktu świata.
		 */
		std::unique_ptr<Entity> cast(float target_x, float target_y) override;

	private:
		std::string _model_path;
		float _model_scale = 1.0f;
		std::shared_ptr<Texture2D> _hit_texture;
	};

} // namespace Nawia::Entity
