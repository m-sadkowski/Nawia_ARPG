#pragma once

#include <Level.h>
#include <LocationDefinition.h>

#include <json.hpp>

#include <initializer_list>
#include <string>
#include <vector>

namespace Nawia::Core {

	/**
	 * @brief Pojedynczy zasob do zaladowania z raportowaniem postepu.
	 */
	struct AssetLoadEntry {
		enum class Kind { Model, MapModel, Animation, Texture } kind = Kind::Model;
		std::string path;
		std::string label;
	};

	/**
	 * @class AssetLoadManifest
	 * @brief Lista unikalnych zasobow do zaladowania przed wejsciem na poziom.
	 */
	class AssetLoadManifest {
	public:
		void addModel(const std::string& path, const std::string& label = "");
		void addMapModel(const std::string& filename, const std::string& label = "");
		void addAnimation(const std::string& path, const std::string& label = "");
		void addTexture(const std::string& path, const std::string& label = "");

		void appendStartupDefaults();
		void appendEntityTypeAssets(const std::string& entity_type, const nlohmann::json& entity_data);

		[[nodiscard]] const std::vector<AssetLoadEntry>& entries() const { return _entries; }
		[[nodiscard]] size_t size() const { return _entries.size(); }
		[[nodiscard]] bool empty() const { return _entries.empty(); }

		/**
		 * @brief Buduje manifest i definicje lokacji z plikow JSON poziomu.
		 */
		static AssetLoadManifest buildForLocationFiles(
			const std::vector<World::LevelLocationFile>& location_files,
			std::vector<World::LocationDefinition>& out_definitions
		);

		/**
		 * @brief Manifest startowy: animacje, modele, mapy ze wszystkich lokacji, ikony UI.
		 */
		static AssetLoadManifest buildStartupManifest();

	private:
		void addUnique(AssetLoadEntry entry);
		void addModelAndAnimation(const std::string& path);
		void addModels(std::initializer_list<const char*> paths);
		void addAnimations(std::initializer_list<const char*> paths);
		void addModelAndAnimations(std::initializer_list<const char*> paths);
		void addTextures(std::initializer_list<const char*> paths);

		std::vector<AssetLoadEntry> _entries;
	};

} // namespace Nawia::Core
