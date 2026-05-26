#include "AssetLoadManifest.h"

#include <LocationJsonLoader.h>
#include <Logger.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>

namespace Nawia::Core {

	namespace {

		bool isPlaceholderMapName(const std::string& model_name) {
			return model_name.empty() || model_name == "placeholder";
		}

		std::string resolveModelPath(std::string model_path) {
			std::ranges::replace(model_path, '\\', '/');
			if (model_path.empty() || model_path.rfind("assets/", 0) == 0)
				return model_path;

			if (std::filesystem::path(model_path).has_parent_path())
				return model_path;

			return "assets/models/" + model_path;
		}

		std::string readStringAlias(const nlohmann::json& data, const std::initializer_list<const char*> keys) {
			for (const char* key : keys) {
				if (data.contains(key) && data[key].is_string())
					return data[key].get<std::string>();
			}
			return "";
		}

		void collectItemIconPaths(std::set<std::string>& paths) {
			const std::filesystem::path items_file = "assets/data/items.json";
			if (!std::filesystem::exists(items_file))
				return;

			std::ifstream file(items_file);
			if (!file.is_open())
				return;

			nlohmann::json data;
			try {
				file >> data;
			} catch (const nlohmann::json::parse_error&) {
				return;
			}

			if (!data.is_array())
				return;

			for (const auto& entry : data) {
				if (!entry.is_object() || !entry.contains("icon"))
					continue;

				const std::string icon = entry["icon"].get<std::string>();
				if (!icon.empty())
					paths.insert(icon);
			}
		}

		void collectLocationMapModelsFromDisk(std::set<std::string>& map_models) {
			const std::filesystem::path locations_dir = "assets/data/locations";
			if (!std::filesystem::exists(locations_dir))
				return;

			for (const auto& entry : std::filesystem::directory_iterator(locations_dir)) {
				if (!entry.is_regular_file() || entry.path().extension() != ".json")
					continue;

				const std::string filename = entry.path().filename().generic_string();
				if (filename.rfind("objects_", 0) == 0)
					continue;

				std::ifstream file(entry.path());
				if (!file.is_open())
					continue;

				nlohmann::json data;
				try {
					file >> data;
				} catch (const nlohmann::json::parse_error&) {
					continue;
				}

				if (!data.contains("map") || !data["map"].is_object())
					continue;

				const std::string model = data["map"].value("model", "");
				if (!isPlaceholderMapName(model))
					map_models.insert(model);
			}
		}

		std::string fileNameFromPath(const std::string& path) {
			return std::filesystem::path(path).filename().string();
		}

	} // namespace

	void AssetLoadManifest::addUnique(AssetLoadEntry entry) {
		if (entry.path.empty())
			return;

		if (entry.label.empty())
			entry.label = fileNameFromPath(entry.path);

		const auto duplicate = std::ranges::find_if(_entries, [&](const AssetLoadEntry& existing) {
			return existing.kind == entry.kind && existing.path == entry.path;
		});
		if (duplicate != _entries.end())
			return;

		_entries.push_back(std::move(entry));
	}

	void AssetLoadManifest::addModel(const std::string& path, const std::string& label) {
		addUnique({AssetLoadEntry::Kind::Model, path, label});
	}

	void AssetLoadManifest::addMapModel(const std::string& filename, const std::string& label) {
		if (isPlaceholderMapName(filename))
			return;

		addUnique({AssetLoadEntry::Kind::MapModel, filename, label.empty() ? filename : label});
	}

	void AssetLoadManifest::addAnimation(const std::string& path, const std::string& label) {
		addUnique({AssetLoadEntry::Kind::Animation, path, label});
	}

	void AssetLoadManifest::addTexture(const std::string& path, const std::string& label) {
		addUnique({AssetLoadEntry::Kind::Texture, path, label});
	}

	void AssetLoadManifest::appendStartupDefaults() {
		for (const char* path : {
			"assets/models/animations/anims.glb",
			"assets/models/animations/anims2.glb",
			"assets/models/player/player_head.glb",
			"assets/models/items/player_head_with_sword.glb",
			"assets/models/cat_bounce.glb",
			"assets/models/chest/chest_close.glb",
			"assets/models/chest/chest_open.glb",
			"assets/models/chest/chest_open_full.glb",
			"assets/models/fireball.glb",
			"assets/models/knife.glb",
			"assets/models/bandit_idle.glb",
			"assets/models/bandit_walk_backwards3.glb",
			"assets/models/bandit_throw.glb",
			"assets/models/bandit_death.glb",
			"assets/models/walking_dead_idle.glb",
			"assets/models/walking_dead_walk.glb",
			"assets/models/walking_dead_run.glb",
			"assets/models/walking_dead_attack.glb",
			"assets/models/walking_dead_death.glb",
			"assets/models/walking_dead_scream.glb",
			"assets/models/walking_dead_hit.glb",
			"assets/models/devil_idle.glb",
			"assets/models/devil_walk.glb",
			"assets/models/devil_run.glb",
			"assets/models/devil_attack.glb",
			"assets/models/devil_dead.glb",
			"assets/models/player_idle.glb",
			"assets/models/player_walk.glb",
			"assets/models/player_auto_attack.glb",
			"assets/models/player_knocked.glb",
			"assets/models/dummy_idle.glb",
			"assets/models/dummy_walk.glb",
			"assets/models/dummy_cast_fireball.glb",
			"assets/models/dummy_death.glb"
		}) {
			addAnimation(path);
			addModel(path);
		}

		std::set<std::string> map_models;
		collectLocationMapModelsFromDisk(map_models);
		for (const auto& map_model : map_models)
			addMapModel(map_model);

		std::set<std::string> icon_paths;
		collectItemIconPaths(icon_paths);
		for (const auto& icon : icon_paths)
			addTexture(icon);

		addTexture("assets/textures/icons/sword_slash_icon.png");
		addTexture("assets/textures/icons/fireball_icon.png");
	}

	void AssetLoadManifest::appendEntityTypeAssets(const std::string& entity_type, const nlohmann::json& entity_data) {
		if (entity_type == "devil") {
			addModel("assets/models/devil_idle.glb");
			addAnimation("assets/models/devil_idle.glb");
			addAnimation("assets/models/devil_walk.glb");
			addAnimation("assets/models/devil_run.glb");
			addAnimation("assets/models/devil_attack.glb");
			addAnimation("assets/models/devil_dead.glb");
			return;
		}

		if (entity_type == "bandit") {
			addModel("assets/models/bandit_idle.glb");
			addAnimation("assets/models/bandit_idle.glb");
			addAnimation("assets/models/bandit_walk_backwards3.glb");
			addAnimation("assets/models/bandit_throw.glb");
			addAnimation("assets/models/bandit_death.glb");
			addModel("assets/models/knife.glb");
			return;
		}

		if (entity_type == "walking_dead") {
			addModel("assets/models/walking_dead_idle.glb");
			addAnimation("assets/models/walking_dead_idle.glb");
			addAnimation("assets/models/walking_dead_walk.glb");
			addAnimation("assets/models/walking_dead_run.glb");
			addAnimation("assets/models/walking_dead_attack.glb");
			addAnimation("assets/models/walking_dead_death.glb");
			addAnimation("assets/models/walking_dead_scream.glb");
			addAnimation("assets/models/walking_dead_hit.glb");
			return;
		}

		if (entity_type == "friend") {
			addModel("assets/models/player_idle.glb");
			addAnimation("assets/models/player_idle.glb");
			addTexture("assets/textures/icons/sword_slash_icon.png");
			return;
		}

		if (entity_type == "chest") {
			addModel("assets/models/chest/chest_close.glb");
			addModel("assets/models/chest/chest_open.glb");
			addModel("assets/models/chest/chest_open_full.glb");
			return;
		}

		if (entity_type == "teleport") {
			addModel("assets/models/fireball.glb");
			return;
		}

		if (entity_type == "npc") {
			addModel("assets/models/cat_bounce.glb");
			addAnimation("assets/models/cat_bounce.glb");
			return;
		}

		if (entity_type == "static_object") {
			const std::string model_path = resolveModelPath(
				readStringAlias(entity_data, {"model", "model_path", "texture"}));
			if (!model_path.empty())
				addModel(model_path);
			return;
		}

		if (entity_type == "boss_trigger") {
			const std::string boss_id = entity_data.value("boss_id", "");
			if (boss_id == "devil" || boss_id.find("devil") != std::string::npos) {
				appendEntityTypeAssets("devil", entity_data);
			}
		}
	}

	AssetLoadManifest AssetLoadManifest::buildForLocationFiles(
		const std::vector<World::LevelLocationFile>& location_files,
		std::vector<World::LocationDefinition>& out_definitions
	) {
		AssetLoadManifest manifest;
		out_definitions.clear();

		for (const auto& location_file : location_files) {
			World::LocationDefinition definition;
			if (!World::LocationJsonLoader::loadLocation(location_file.path, definition))
				continue;

			if (!location_file.name.empty())
				definition.name = location_file.name;

			out_definitions.push_back(definition);
			manifest.addMapModel(definition.map.model, "Mapa: " + definition.name);

			for (const auto& entity_data : definition.entities) {
				const std::string entity_type = entity_data.value("type", "");
				if (entity_type.empty())
					continue;

				manifest.appendEntityTypeAssets(entity_type, entity_data);

				const int spawn_count = std::max(1, entity_data.value("count", 1));
				if (spawn_count > 1) {
					// count nie zmienia zestawu modeli, tylko liczbe instancji
				}
			}
		}

		return manifest;
	}

	AssetLoadManifest AssetLoadManifest::buildStartupManifest() {
		AssetLoadManifest manifest;
		manifest.appendStartupDefaults();
		return manifest;
	}

} // namespace Nawia::Core
