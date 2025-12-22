#include "ResourceManager.h"
#include "Logger.h"

#include <iostream>

namespace Nawia::Core {

	std::shared_ptr<Texture2D> ResourceManager::getTexture(const std::string& filename) 
	{
		// if texture is already loaded, return it
		// if not, load it from file
		const auto texture = _textures.find(filename);
		if (texture != _textures.end())
			return texture->second;

		// load texture using raylib
		const Texture2D tex = LoadTexture(filename.c_str());

		if (tex.id == 0) {
			Logger::errorLog("Resource Manager - could not load texture: " + filename);
			return nullptr;
		}

		Logger::debugLog("Resource Manager - loaded file: " + filename);

		// create a shared_ptr to put in map, custom deleter calls UnloadTexture
		std::shared_ptr<Texture2D> new_texture(new Texture2D(tex), [](const Texture2D *t) 
			{
			UnloadTexture(*t);
			delete t;
			Logger::debugLog("Resource Manager - removed resource.");
			}
		);

		_textures[filename] = new_texture;
		return new_texture;
	}

} // namespace Nawia::Core