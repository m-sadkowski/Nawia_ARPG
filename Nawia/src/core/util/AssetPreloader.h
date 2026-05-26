#pragma once

#include "AssetLoadManifest.h"

#include <functional>
#include <string>

namespace Nawia::Core {

	class ResourceManager;

	/**
	 * @class AssetPreloader
	 * @brief Laduje manifest zasobow i raportuje postep po kazdym elemencie.
	 */
	class AssetPreloader {
	public:
		using ProgressCallback = std::function<void(float progress, const std::string& status_label)>;

		/**
		 * @brief Laduje jeden wpis manifestu (model, animacja, mapa, tekstura).
		 */
		static bool loadEntry(
			const AssetLoadEntry& entry,
			ResourceManager& resource_manager
		);

		/**
		 * @brief Laduje caly manifest; po kazdym elemencie wywoluje callback postepu.
		 */
		static void loadManifest(
			const AssetLoadManifest& manifest,
			ResourceManager& resource_manager,
			ProgressCallback on_progress
		);

		/**
		 * @brief Laduje pojedynczy element manifestu po indeksie.
		 */
		static bool loadManifestStep(
			const AssetLoadManifest& manifest,
			size_t index,
			ResourceManager& resource_manager
		);
	};

} // namespace Nawia::Core
