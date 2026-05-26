#include "ResourceManager.h"

#include <Logger.h>

#include <raymath.h>

#include <cstdlib>
#include <cstring>

namespace Nawia::Core {

// Raylib definiuje MAX_MATERIAL_MAPS w wewnetrznym config.h (domyslnie 12).
// Nie jest eksponowane w publicznym raylib.h, wiec definiujemy lokalna kopie.
constexpr int k_max_material_maps = 12;

namespace {

	/**
	 * @brief Kopiuje bufor surowych bajtow. Zwraca nullptr jesli src == nullptr.
	 */
	void* cloneBuffer(const void* src, const size_t size_bytes) {
		if (!src || size_bytes == 0) return nullptr;
		void* dst = std::malloc(size_bytes);
		if (dst) std::memcpy(dst, src, size_bytes);
		return dst;
	}

	/**
	 * @brief Tworzy gleboka kopie siatki — nowe bufory CPU + nowe VBO na GPU.
	 *
	 * Kopiowane sa wszystkie pola Mesh wlacznie z animVertices / animNormals
	 * (buforami zmienianymi przez UpdateModelAnimation). Nastepnie UploadMesh
	 * tworzy nowy VAO i VBO, dzieki czemu kazda encja moze animowac niezaleznie.
	 */
	Mesh cloneMeshData(const Mesh& src) {
		Mesh dst = {};
		dst.vertexCount = src.vertexCount;
		dst.triangleCount = src.triangleCount;

		const auto vc = static_cast<size_t>(src.vertexCount);

		dst.vertices     = static_cast<float*>(cloneBuffer(src.vertices,     vc * 3 * sizeof(float)));
		dst.texcoords    = static_cast<float*>(cloneBuffer(src.texcoords,    vc * 2 * sizeof(float)));
		dst.texcoords2   = static_cast<float*>(cloneBuffer(src.texcoords2,   vc * 2 * sizeof(float)));
		dst.normals      = static_cast<float*>(cloneBuffer(src.normals,      vc * 3 * sizeof(float)));
		dst.tangents     = static_cast<float*>(cloneBuffer(src.tangents,     vc * 4 * sizeof(float)));
		dst.colors       = static_cast<unsigned char*>(cloneBuffer(src.colors, vc * 4));

		if (src.indices) {
			const auto tc = static_cast<size_t>(src.triangleCount);
			dst.indices = static_cast<unsigned short*>(cloneBuffer(src.indices, tc * 3 * sizeof(unsigned short)));
		}

		// ---- Animacja: animVertices / animNormals ----
		// Model w cache mogl nie byc jeszcze animowany, wiec animVertices/animNormals
		// moga byc NULL. Jesli siatka ma dane kosciowe, musimy je preallokowac — inaczej
		// UpdateModelAnimation uderzy w nullptr przy pierwszym uzyciu.
		if (src.animVertices) {
			dst.animVertices = static_cast<float*>(cloneBuffer(src.animVertices, vc * 3 * sizeof(float)));
		} else if (src.boneIds && src.vertices) {
			// Prealokacja z kopii bind-pose.
			dst.animVertices = static_cast<float*>(cloneBuffer(src.vertices, vc * 3 * sizeof(float)));
		}

		if (src.animNormals) {
			dst.animNormals = static_cast<float*>(cloneBuffer(src.animNormals, vc * 3 * sizeof(float)));
		} else if (src.boneIds && src.normals) {
			dst.animNormals = static_cast<float*>(cloneBuffer(src.normals, vc * 3 * sizeof(float)));
		}

		dst.boneIds      = static_cast<unsigned char*>(cloneBuffer(src.boneIds,      vc * 4));
		dst.boneWeights  = static_cast<float*>(cloneBuffer(src.boneWeights,  vc * 4 * sizeof(float)));

		// ---- GPU skinning: boneMatrices (raylib 5.5+) ----
		// UpdateModelAnimationBones zapisuje transformacje do mesh.boneMatrices[boneId].
		// Musimy sklonowac ten bufor, inaczej animacja sie wysypie.
		dst.boneCount = src.boneCount;
		if (src.boneMatrices && src.boneCount > 0) {
			dst.boneMatrices = static_cast<Matrix*>(cloneBuffer(
				src.boneMatrices, static_cast<size_t>(src.boneCount) * sizeof(Matrix)));
		} else if (src.boneCount > 0) {
			// Prealokacja zerowych macierzy — UpdateModelAnimationBones je uzupelni.
			dst.boneMatrices = static_cast<Matrix*>(
				std::calloc(static_cast<size_t>(src.boneCount), sizeof(Matrix)));
		}

		// Nowe identyfikatory GPU — UploadMesh utworzy wlasny VAO i zestaw VBO.
		dst.vaoId = 0;
		dst.vboId = nullptr;
		UploadMesh(&dst, false);

		return dst;
	}

} // namespace

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
			_model_cache[path] = model;
			return &_model_cache[path];
		}
		Logger::errorLog("ResourceManager: nie udalo sie zaladowac modelu: " + path);
		return nullptr;
	}

	Model ResourceManager::cloneModel(const std::string& path) {
		// Upewnij sie, ze zrodlo jest w cache.
		Model* source = getModel(path);
		if (!source || source->meshCount == 0)
			return {};

		Model clone = {};
		clone.transform = source->transform;
		clone.meshCount = source->meshCount;
		clone.materialCount = source->materialCount;

		// ---- Meshes: pelna gleboka kopia (wlasne bufory CPU + GPU) ----
		clone.meshes = static_cast<Mesh*>(std::calloc(source->meshCount, sizeof(Mesh)));
		for (int i = 0; i < source->meshCount; i++)
			clone.meshes[i] = cloneMeshData(source->meshes[i]);

		// ---- Materials: plytka kopia — tekstury to uchwyty GPU (read-only) ----
		clone.materials = static_cast<Material*>(std::malloc(source->materialCount * sizeof(Material)));
		std::memcpy(clone.materials, source->materials, source->materialCount * sizeof(Material));

		// Kazdy Material ma wlasny uchwyt maps[]; kopiujemy go, zeby UnloadClonedModel
		// mogl zwolnic tablice maps bez ryzyka double-free.
		for (int i = 0; i < source->materialCount; i++) {
			if (source->materials[i].maps) {
				clone.materials[i].maps = static_cast<MaterialMap*>(
					std::malloc(k_max_material_maps * sizeof(MaterialMap)));
				std::memcpy(clone.materials[i].maps, source->materials[i].maps,
					k_max_material_maps * sizeof(MaterialMap));
			}
		}

		// ---- Mesh-material mapping ----
		clone.meshMaterial = static_cast<int*>(std::malloc(source->meshCount * sizeof(int)));
		std::memcpy(clone.meshMaterial, source->meshMaterial, source->meshCount * sizeof(int));

		// ---- Bones / bind pose ----
		clone.boneCount = source->boneCount;
		if (source->boneCount > 0 && source->bones && source->bindPose) {
			clone.bones = static_cast<BoneInfo*>(
				std::malloc(source->boneCount * sizeof(BoneInfo)));
			std::memcpy(clone.bones, source->bones, source->boneCount * sizeof(BoneInfo));

			clone.bindPose = static_cast<Transform*>(
				std::malloc(source->boneCount * sizeof(Transform)));
			std::memcpy(clone.bindPose, source->bindPose, source->boneCount * sizeof(Transform));
		}

		return clone;
	}

	void ResourceManager::unloadClonedModel(Model& model) {
		// Zerujemy ID tekstur, zeby UnloadModel nie usunelo zasobow GPU nalezacych
		// do cache'u ResourceManagera. W OpenGL glDeleteTextures(1, &0) to no-op.
		for (int i = 0; i < model.materialCount; i++) {
			if (model.materials[i].maps) {
				for (int j = 0; j < k_max_material_maps; j++)
					model.materials[i].maps[j].texture.id = 0;
			}
		}

		UnloadModel(model);
		model = {};
	}

} // namespace Nawia::Core
