#include "ResourceManager.h"

namespace Nawia::Core {

	std::shared_ptr<SDL_Texture> ResourceManager::getTexture(const std::string& filename, SDL_Renderer* renderer) {
		// if texture is already loaded, return it
		if (!_textures.empty()) {
			auto texture = _textures.find(filename);
			if (texture != _textures.end()) {
				return texture->second;
			}
		}

		// if not, load it from file

		// open file
		SDL_IOStream* stream = SDL_IOFromFile(filename.c_str(), "r");
		if (!stream) {
			// ERROR - couldnt open the file
			std::cerr << "[ERROR] Resource Manager - couldn't open file: '" << filename.c_str() << "'\n";
			return nullptr;
		}
		// DEBUG
		printf("[DEBUG] Resource Manager - opened file: '%s'", filename.c_str());

		// create a SDL Surface
		SDL_Surface* surface = IMG_LoadPNG_IO(stream);
		if (!surface) {
			// ERROR - couldnt create a surface
			printf("[ERROR] Resource Manager - couldn't create a surface");
			return nullptr;
		}

		// create a texture
		SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_DestroySurface(surface);

		// create a sharedptr
		std::shared_ptr<SDL_Texture> newTexture(tex, [](SDL_Texture* t) {
			// destroy texture when not needed
			SDL_DestroyTexture(t);
			});

		_textures[filename] = newTexture;
		return newTexture;
	}

}