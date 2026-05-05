#include "ResourceManager.h"

#include <Logger.h>

namespace Nawia::Core {

	std::shared_ptr<Texture2D> ResourceManager::getTexture(const std::string& filename) {
		const auto cached_texture = _textures.find(filename);
		if (cached_texture != _textures.end())
			return cached_texture->second;

		const Texture2D texture = LoadTexture(filename.c_str());
		if (texture.id == 0) {
			Logger::errorLog("ResourceManager: nie udalo sie zaladowac tekstury: " + filename);
			return nullptr;
		}

		std::shared_ptr<Texture2D> loaded_texture(new Texture2D(texture), [](const Texture2D* texture_to_unload) {
			UnloadTexture(*texture_to_unload);
			delete texture_to_unload;
		});

		_textures[filename] = loaded_texture;
		return loaded_texture;
	}

} // namespace Nawia::Core
