#include "EntityAnimationSupport.h"

#include <raymath.h>

#include <cstdlib>
#include <cstring>
#include <map>

namespace Nawia::Entity {
namespace {

	std::map<std::string, std::shared_ptr<const AnimationBundle>> g_animation_cache;

	void* cloneRawBuffer(const void* src, const size_t size_bytes)
	{
		if (!src || size_bytes == 0)
			return nullptr;

		void* dst = std::malloc(size_bytes);
		if (dst)
			std::memcpy(dst, src, size_bytes);
		return dst;
	}

}

	AnimationBundle::~AnimationBundle()
	{
		for (const auto& anim : clips)
			UnloadModelAnimation(anim);
	}

	void ensureMeshAnimationBuffers(Model& model)
	{
		if (model.meshCount <= 0 || model.boneCount <= 0)
			return;

		for (int mesh_index = 0; mesh_index < model.meshCount; ++mesh_index) {
			Mesh& mesh = model.meshes[mesh_index];
			const auto vertex_count = static_cast<size_t>(mesh.vertexCount);
			if (vertex_count == 0)
				continue;

			if (!mesh.animVertices && mesh.vertices)
				mesh.animVertices = static_cast<float*>(cloneRawBuffer(mesh.vertices, vertex_count * 3 * sizeof(float)));

			if (!mesh.animNormals && mesh.normals)
				mesh.animNormals = static_cast<float*>(cloneRawBuffer(mesh.normals, vertex_count * 3 * sizeof(float)));

			if (!mesh.boneIds)
				mesh.boneIds = static_cast<unsigned char*>(std::calloc(vertex_count * 4, sizeof(unsigned char)));

			if (!mesh.boneWeights) {
				mesh.boneWeights = static_cast<float*>(std::calloc(vertex_count * 4, sizeof(float)));
				if (mesh.boneWeights) {
					for (size_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
						mesh.boneWeights[vertex_index * 4] = 1.0f;
				}
			}

			if (mesh.boneCount <= 0)
				mesh.boneCount = model.boneCount;

			if (!mesh.boneMatrices && mesh.boneCount > 0) {
				mesh.boneMatrices = static_cast<Matrix*>(std::calloc(static_cast<size_t>(mesh.boneCount), sizeof(Matrix)));
				if (mesh.boneMatrices) {
					for (int bone_index = 0; bone_index < mesh.boneCount; ++bone_index)
						mesh.boneMatrices[bone_index] = MatrixIdentity();
				}
			}
		}
	}

	std::shared_ptr<const AnimationBundle> getCachedAnimationBundle(const std::string& path)
	{
		const auto cached_animation = g_animation_cache.find(path);
		if (cached_animation != g_animation_cache.end())
			return cached_animation->second;

		int count = 0;
		ModelAnimation* anims = LoadModelAnimations(path.c_str(), &count);

		auto bundle = std::make_shared<AnimationBundle>();
		if (count > 0 && anims != nullptr) {
			bundle->clips.reserve(count);
			for (int i = 0; i < count; i++)
				bundle->clips.push_back(anims[i]);
		}

		if (anims != nullptr)
			MemFree(anims);

		g_animation_cache[path] = bundle;
		return bundle;
	}

	const ModelAnimation* resolveAnimation(const AnimationClipRef& animation)
	{
		if (!animation.bundle)
			return nullptr;

		if (animation.clip_index < 0 || static_cast<size_t>(animation.clip_index) >= animation.bundle->clips.size())
			return nullptr;

		return &animation.bundle->clips[animation.clip_index];
	}

}
