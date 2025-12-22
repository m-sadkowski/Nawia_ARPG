#pragma once
#include <raylib.h>
#include <map>
#include <memory>
#include <string>


namespace Nawia::Core {

	class ResourceManager {
	public:
		// returns a shared_ptr to a Texture2D, the last shared_ptr dies, the custom deleter will call UnloadTexture.
		std::shared_ptr<Texture2D> getTexture(const std::string& filename);

	private:
		std::map<std::string, std::shared_ptr<Texture2D>> _textures;
	};

} // namespace Nawia::Core