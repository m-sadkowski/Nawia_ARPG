#pragma once

#include <LocationDefinition.h>

#include <json.hpp>

#include <filesystem>

namespace Nawia::World {

	class LocationJsonLoader {
	public:
		/**
		 * @brief Wczytuje plik lokacji zapisany przez kreator poziomow.
		 */
		[[nodiscard]] static bool loadLocation(
			const std::filesystem::path& location_path,
			LocationDefinition& out_location
		);

		/**
		 * @brief Buduje tymczasowy JSON w formacie zrozumialym dla SpawnManagera.
		 */
		[[nodiscard]] static nlohmann::json buildSpawnRoot(const LocationDefinition& location);
	};

} // namespace Nawia::World
