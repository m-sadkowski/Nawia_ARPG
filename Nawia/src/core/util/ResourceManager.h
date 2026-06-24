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
		~ResourceManager();

		std::shared_ptr<Texture2D> getTexture(const std::string& filename);
		Model* getModel(const std::string& path);

		/**
		 * @brief Klonuje model z cache'u — nowe bufory mesh (wlasne VBO),
		 *        wspoldzielone tekstury GPU. ~100x szybsze niz LoadModel z dysku.
		 *
		 * Zwracany Model nalezy zwolnic przez unloadClonedModel(), NIE UnloadModel().
		 * @return Model z wlasnymi buforami siatki, albo pusty Model jesli brak w cache.
		 */
		Model cloneModel(const std::string& path);

		/**
		 * @brief Zwalnia sklonowany model: meshe i kosciowy, ale NIE tekstury.
		 */
		static void unloadClonedModel(Model& model);

		void clear();

	private:
		std::map<std::string, std::shared_ptr<Texture2D>> _textures;
		std::map<std::string, Model> _model_cache;
	};

} // namespace Nawia::Core
