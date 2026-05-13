#pragma once

#include <raylib.h>

#include <map>
#include <memory>
#include <string>

namespace Nawia::Core {

	/**
	 * @class ResourceManager
	 * @brief Laduje i wspoldzieli tekstury Raylib.
	 *
	 * Manager posiada cache tekstur. Zwracany `shared_ptr` pozwala UI i encjom
	 * bezpiecznie korzystac z tej samej tekstury bez recznego wywolywania
	 * `UnloadTexture`.
	 */
	class ResourceManager {
	public:
		/**
		 * @brief Zwraca teksture z cache albo laduje ja z pliku.
		 */
		~ResourceManager() {
			for (auto& pair : _model_cache) {
				UnloadModel(pair.second);
			}
			_model_cache.clear();
		}

		std::shared_ptr<Texture2D> getTexture(const std::string& filename);
		Model* getModel(const std::string& path);

	private:
		std::map<std::string, std::shared_ptr<Texture2D>> _textures;
		std::map<std::string, Model> _model_cache;
	};

} // namespace Nawia::Core
