#include "ResourceManager.h"

#include <Logger.h>

#include <raymath.h>

namespace Nawia::Core {

	ResourceManager::~ResourceManager() {
		clear();
	}

	void ResourceManager::clear() {
		for (auto& pair : _model_cache)
			UnloadModel(pair.second);

		_model_cache.clear();
		_textures.clear();
	}

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

	Model* ResourceManager::getModel(const std::string& path) {
		if (_model_cache.count(path)) {
			return &_model_cache[path];
		}

		// Jeśli go nie ma, załaduj z dysku i zapisz w słowniku
		Model model = LoadModel(path.c_str());
		if (model.meshCount > 0) {
			// Tu od razu możesz dać poprawkę na rotację
			model.transform = MatrixRotateY(PI / 2.0f);
			_model_cache[path] = model;
			return &_model_cache[path];
		}
		Logger::errorLog("ResourceManager: nie udalo sie zaladowac modelu: " + path);
		return nullptr;
	}

} // namespace Nawia::Core
