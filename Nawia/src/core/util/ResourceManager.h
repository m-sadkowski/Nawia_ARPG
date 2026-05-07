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
		std::shared_ptr<Texture2D> getTexture(const std::string& filename);

	private:
		std::map<std::string, std::shared_ptr<Texture2D>> _textures;
	};

} // namespace Nawia::Core
