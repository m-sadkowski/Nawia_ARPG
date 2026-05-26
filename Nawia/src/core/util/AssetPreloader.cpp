#include "AssetPreloader.h"

#include <Entity.h>
#include <Logger.h>
#include <Map.h>
#include <ResourceManager.h>

namespace Nawia::Core {

	bool AssetPreloader::loadEntry(const AssetLoadEntry& entry, ResourceManager& resource_manager) {
		switch (entry.kind) {
			case AssetLoadEntry::Kind::Model:
				return resource_manager.getModel(entry.path) != nullptr;
			case AssetLoadEntry::Kind::MapModel:
				Map::preloadMapModel(entry.path);
				return true;
			case AssetLoadEntry::Kind::Animation:
				Entity::Entity::preloadAnimationData(entry.path);
				return true;
			case AssetLoadEntry::Kind::Texture:
				return resource_manager.getTexture(entry.path) != nullptr;
		}

		return false;
	}

	void AssetPreloader::loadManifest(
		const AssetLoadManifest& manifest,
		ResourceManager& resource_manager,
		ProgressCallback on_progress
	) {
		const size_t total = manifest.size();
		if (total == 0) {
			if (on_progress)
				on_progress(1.0f, "Gotowe");
			return;
		}

		for (size_t index = 0; index < total; ++index) {
			const auto& entry = manifest.entries()[index];
			loadEntry(entry, resource_manager);

			if (on_progress) {
				const float progress = static_cast<float>(index + 1) / static_cast<float>(total);
				on_progress(progress, entry.label);
			}
		}
	}

	bool AssetPreloader::loadManifestStep(
		const AssetLoadManifest& manifest,
		const size_t index,
		ResourceManager& resource_manager
	) {
		if (index >= manifest.size())
			return false;

		return loadEntry(manifest.entries()[index], resource_manager);
	}

} // namespace Nawia::Core
