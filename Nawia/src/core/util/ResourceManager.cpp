#include "ResourceManager.h"

#include <iostream>
#include <SDL3_image/SDL_image.h>

#include "Logger.h"

namespace Nawia::Core {

	std::shared_ptr<SDL_Texture> ResourceManager::getTexture(const std::string& filename, SDL_Renderer* renderer) {
		// if texture is already loaded, return it
		// if not, load it from file
		const auto texture = _textures.find(filename);
		if (texture != _textures.end()) {
			return texture->second;
		}

		// open file
		SDL_IOStream* stream = SDL_IOFromFile(filename.c_str(), "r");
		if (!stream) {
			Logger::errorLog("Resource Manager - could not open file: " + filename);
			Logger::errorLog("SDL: " + std::string(SDL_GetError()));
			return nullptr;
		}
		Logger::debugLog("Resource Manager - opened file:: " + filename);

		// create a SDL Surface
		SDL_Surface* surface = IMG_LoadPNG_IO(stream);
		if (!surface) {
			Logger::errorLog("Resource Manager - could not create a surface.");
			Logger::errorLog("SDL: " + std::string(SDL_GetError()));
			return nullptr;
		}

		// create a texture
		SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_DestroySurface(surface);

		// create a shared_ptr to put in map
		std::shared_ptr<SDL_Texture> new_texture(tex, [](SDL_Texture* t) {
			// destroy texture when not needed
			SDL_DestroyTexture(t);
			Logger::debugLog("Resource Manager - removed resource.");
			});

		_textures[filename] = new_texture;
		return new_texture;
	}

} // Nawia::Core