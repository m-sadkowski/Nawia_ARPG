#pragma once

#include <string>
#include <map>
#include <memory>
#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

namespace Nawia::Core {
	
	class ResourceManager {
	public:
		std::shared_ptr<SDL_Texture> getTexture(const std::string& filename, SDL_Renderer* renderer);

	private:
		std::map<std::string, std::shared_ptr<SDL_Texture>> _textures;
	};

}