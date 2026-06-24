#pragma once

#include <ProjectileAbility.h>

#include <raylib.h>

#include <memory>
#include <string>

namespace Nawia::Entity {

	/**
	 * @class KnifeThrowAbility
	 * @brief Dystansowa umiejętność rzutu nożem dla bandyty.
	 *
	 * Tworzy pocisk 3D lecący w stronę wskazanego celu.
	 */
	class KnifeThrowAbility : public ProjectileAbility {
	public:
		/**
		 * @brief Tworzy rzut nożem z modelem, skalą i przesunięciem kierunku modelu.
		 */
		KnifeThrowAbility(const std::string& model_path,
						  float model_scale,
						  const std::shared_ptr<Texture2D>& hit_tex,
						  const std::shared_ptr<Texture2D>& icon_tex,
						  float facing_offset = 0.0f,
						  Core::ResourceManager* resource_manager = nullptr);
	};

} // namespace Nawia::Entity
