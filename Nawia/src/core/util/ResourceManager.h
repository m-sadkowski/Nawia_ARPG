#pragma once

#include <string>
#include <map>
#include <memory>
#include <SDL3/SDL.h>

namespace Nawia::Core {
	
	class ResourceManager {
	public:
		std::shared_ptr<SDL_Texture> getTexture(const std::string& filename, SDL_Renderer* renderer);

	private:
		std::map<std::string, std::shared_ptr<SDL_Texture>> _textures;
	};

} // Nawia::Core