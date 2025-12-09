#include "ResourceManager.h"

namespace Nawia::Core {

	std::shared_ptr<SDL_Texture> ResourceManager::getTexture(const std::string& filename, SDL_Renderer* renderer) {
		// if texture is already loaded, return it
		auto texture = _textures.find(filename);
		if (texture != _textures.end()) {
			return texture->second;
		}

		// if not, load it from file

		// open file
		SDL_IOStream* stream = SDL_IOFromFile(filename.c_str(), "r");
		if (!stream) {
			std::cerr << "[ERROR] Resource Manager - couldn't open file: '" << filename.c_str() << "'\n";
			std::cerr << "[ERROR] SDL: " << SDL_GetError() << "\n";
			return nullptr;
		}
		// DEBUG
		std::cout << "[DEBUG] Resource Manager - opened file: '" << filename.c_str() << "'\n";

		// create a SDL Surface
		SDL_Surface* surface = IMG_LoadPNG_IO(stream);
		if (!surface) {
			std::cerr << "[ERROR] Resource Manager - couldn't create a surface\n";
			std::cerr << "[ERROR] SDL: " << SDL_GetError() << "\n";
			return nullptr;
		}

		// create a texture
		SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_DestroySurface(surface);

		// create a sharedptr to put in map
		std::shared_ptr<SDL_Texture> newTexture(tex, [](SDL_Texture* t) {
			// destroy texture when not needed
			SDL_DestroyTexture(t);
			// DEBUG
			std::cout << "[DEBUG] Resource Manager - removed resource\n";
			});

		_textures[filename] = newTexture;
		return newTexture;
	}

}