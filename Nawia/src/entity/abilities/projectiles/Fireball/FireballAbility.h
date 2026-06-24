#pragma once

#include <ProjectileAbility.h>

#include <raylib.h>

#include <memory>
#include <string>

namespace Nawia::Entity {

	/**
	 * @class FireballAbility
	 * @brief Dystansowa umiejętność tworząca pocisk ognistej kuli.
	 */
	class FireballAbility : public ProjectileAbility {
	public:
		/**
		 * @brief Tworzy umiejętność z modelem pocisku, skalą i teksturami UI/trafienia.
		 */
		FireballAbility(const std::string& model_path,
						float model_scale,
						const std::shared_ptr<Texture2D>& hit_tex,
						const std::shared_ptr<Texture2D>& icon_tex,
						Core::ResourceManager* resource_manager = nullptr);

	protected:
		/**
     * @brief Fireball startuje z logicznej pozycji źródła użycia, jak w dotychczasowym zachowaniu.
		 */
		[[nodiscard]] Vector2 getSpawnPosition() const override;
	};

} // namespace Nawia::Entity
