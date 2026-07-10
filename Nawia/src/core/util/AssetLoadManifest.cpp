#include "AssetLoadManifest.h"

#include <AssetPathUtils.h>
#include <JsonUtils.h>
#include <LocationJsonLoader.h>
#include <Logger.h>

#include <filesystem>
#include <fstream>
#include <set>

namespace Nawia::Core {

	namespace {

		bool isPlaceholderMapName(const std::string& model_name) {
			return model_name.empty() || model_name == "placeholder";
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
			if (!entry.is_object())
				continue;

			std::string icon;
			if (entry.contains("icon") && entry["icon"].is_string())
				icon = entry["icon"].get<std::string>();
			else if (entry.contains("texture") && entry["texture"].is_string())
				icon = entry["texture"].get<std::string>();

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

	void AssetLoadManifest::addModelAndAnimation(const std::string& path) {
		addModel(path);
		addAnimation(path);
	}

	void AssetLoadManifest::addModels(const std::initializer_list<const char*> paths) {
		for (const char* path : paths)
			addModel(path);
	}

	void AssetLoadManifest::addAnimations(const std::initializer_list<const char*> paths) {
		for (const char* path : paths)
			addAnimation(path);
	}

	void AssetLoadManifest::addModelAndAnimations(const std::initializer_list<const char*> paths) {
		for (const char* path : paths)
			addModelAndAnimation(path);
	}

	void AssetLoadManifest::addTextures(const std::initializer_list<const char*> paths) {
		for (const char* path : paths)
			addTexture(path);
	}

	void AssetLoadManifest::appendStartupDefaults() {
		addModelAndAnimations({
			"assets/models/animations/anims.glb",
			"assets/models/animations/anims2.glb",
			"assets/models/actors/player/parts/player_head.glb",
			"assets/models/items/player_head_with_sword.glb",
			"assets/models/actors/cat/cat_bounce.glb",
			"assets/models/chest/chest_close.glb",
			"assets/models/chest/chest_open.glb",
			"assets/models/chest/chest_open_full.glb",
			"assets/models/fireball.glb",
			"assets/models/knife.glb",
			"assets/models/actors/bandit/bandit_idle.glb",
			"assets/models/actors/bandit/bandit_walk_backwards3.glb",
			"assets/models/actors/bandit/bandit_throw.glb",
			"assets/models/actors/bandit/bandit_death.glb",
			"assets/models/actors/walking_dead/walking_dead.glb",
			"assets/models/actors/walking_dead/walking_dead_2.glb",
			"assets/models/actors/frog/frog.glb",
			"assets/models/actors/worm/worm.glb",
			"assets/models/actors/spider/spider.glb",
			"assets/models/cobweb.glb",
			"assets/models/teleport.glb",
			"assets/models/actors/mini_mushroom/mini_mushroom.glb",
			"assets/models/actors/mini_mushroom/mini_mushroom_infected.glb",
			"assets/models/actors/mushroom/mushroom_raylib_fixed.glb",
			"assets/models/actors/village_head/village_head.glb",
			"assets/models/actors/baba_yaga/baba_yaga.glb",
			"assets/models/actors/wanda/woman_dress.glb",
			"assets/models/actors/devil/devil_idle.glb",
			"assets/models/actors/devil/devil_walk.glb",
			"assets/models/actors/devil/devil_run.glb",
			"assets/models/actors/devil/devil_attack.glb",
			"assets/models/actors/devil/devil_dead.glb",
			"assets/models/actors/dragon/dragon.glb",
			"assets/models/actors/player/player_idle.glb",
			"assets/models/actors/player/player_walk.glb",
			"assets/models/actors/player/player_auto_attack.glb",
			"assets/models/actors/player/player_knocked.glb"
		});

		std::set<std::string> map_models;
		collectLocationMapModelsFromDisk(map_models);
		for (const auto& map_model : map_models)
			addMapModel(map_model);

		std::set<std::string> icon_paths;
		collectItemIconPaths(icon_paths);
		for (const auto& icon : icon_paths)
			addTexture(icon);

		addTextures({
			"assets/textures/icons/sword_slash_icon.png",
			"assets/textures/icons/fireball_icon.png",
			"assets/textures/icons/punch_icon.png",
			"assets/textures/icons/strong_hit_icon.png",
			"assets/textures/icons/empty_ability_icon.png",
			"assets/textures/icons/food_icon.png",
			"assets/textures/outro/odgruzowanie_przejscia.png",
			"assets/textures/outro/dziady_przy_potoku.png",
			"assets/textures/outro/droga_do_twierdzy.png"
		});
	}

	void AssetLoadManifest::appendEntityTypeAssets(const std::string& entity_type, const nlohmann::json& entity_data) {
		if (entity_type == "devil") {
			addModel("assets/models/actors/devil/devil_idle.glb");
			addAnimations({
				"assets/models/actors/devil/devil_idle.glb",
				"assets/models/actors/devil/devil_walk.glb",
				"assets/models/actors/devil/devil_run.glb",
				"assets/models/actors/devil/devil_attack.glb",
				"assets/models/actors/devil/devil_dead.glb"
			});
			return;
		}

		if (entity_type == "rift_binder" || entity_type == "dragon") {
			addModelAndAnimation("assets/models/actors/dragon/dragon.glb");
			appendEntityTypeAssets("walking_dead", entity_data);
			addModels({
				"assets/models/actors/walking_dead/walking_dead_2.glb",
				"assets/models/fireball.glb",
				"assets/models/totem.glb"
			});
			return;
		}

		if (entity_type == "bandit") {
			addModel("assets/models/actors/bandit/bandit_idle.glb");
			addAnimations({
				"assets/models/actors/bandit/bandit_idle.glb",
				"assets/models/actors/bandit/bandit_walk_backwards3.glb",
				"assets/models/actors/bandit/bandit_throw.glb",
				"assets/models/actors/bandit/bandit_death.glb"
			});
			addModel("assets/models/knife.glb");
			return;
		}

		if (entity_type == "walking_dead") {
			addModelAndAnimation("assets/models/actors/walking_dead/walking_dead.glb");
			return;
		}

		if (entity_type == "frog") {
			addModelAndAnimations({
				"assets/models/actors/frog/frog.glb",
				"assets/models/actors/village_head/village_head.glb"
			});
			return;
		}

		if (entity_type == "worm") {
			addModelAndAnimation("assets/models/actors/worm/worm.glb");
			return;
		}

		if (entity_type == "spider") {
			addModelAndAnimation("assets/models/actors/spider/spider.glb");
			addModel("assets/models/cobweb.glb");
			return;
		}

		if (entity_type == "mini_mushroom_infected") {
			addModelAndAnimations({
				"assets/models/actors/mini_mushroom/mini_mushroom_infected.glb",
				"assets/models/actors/mini_mushroom/mini_mushroom.glb",
				"assets/models/actors/worm/worm.glb"
			});
			return;
		}

		if (entity_type == "friend") {
			addModelAndAnimation("assets/models/actors/player/player_idle.glb");
			addTextures({
				"assets/textures/icons/sword_slash_icon.png",
				"assets/textures/icons/punch_icon.png",
				"assets/textures/icons/strong_hit_icon.png",
				"assets/textures/icons/empty_ability_icon.png",
				"assets/textures/icons/food_icon.png"
			});
			return;
		}

		if (entity_type == "chest") {
			addModels({
				"assets/models/chest/chest_close.glb",
				"assets/models/chest/chest_open.glb",
				"assets/models/chest/chest_open_full.glb"
			});
			return;
		}

		if (entity_type == "teleport") {
			addModel("assets/models/teleport.glb");
			return;
		}

		if (entity_type == "witch") {
			addModelAndAnimation("assets/models/actors/witch/witch.glb");
			addModel("assets/models/fireball.glb");
			appendEntityTypeAssets("walking_dead", entity_data);
			return;
		}

		if (entity_type == "npc") {
			const std::string npc_class = entity_data.value("npc_class", "cat");
			if (npc_class == "mushroom") {
				addModelAndAnimation("assets/models/actors/mushroom/mushroom_raylib_fixed.glb");
			} else if (npc_class == "szeptucha") {
				addModel("assets/models/actors/baba_yaga/baba_yaga.glb");
			} else if (npc_class == "village_head") {
				addModelAndAnimation("assets/models/actors/village_head/village_head.glb");
			} else if (npc_class == "wanda_corpse") {
				addModel("assets/models/actors/wanda/woman_dress.glb");
			} else if (npc_class == "forest_lost_group") {
				addModelAndAnimations({
					"assets/models/actors/npcs/female_npc_2.glb",
					"assets/models/actors/npcs/male_npc_2.glb",
					"assets/models/actors/milena_sister/milena_sister.glb"
				});
			} else if (npc_class == "cemetery_survivor_group") {
				addModelAndAnimations({
					"assets/models/actors/friends/female_warrior/female_warrior.glb",
					"assets/models/actors/npcs/male_npc_1.glb"
				});
			} else if (npc_class == "story_human" || npc_class == "herbalist") {
				const std::string model_path = AssetPathUtils::resolveModelPath(
					JsonUtils::readStringAlias(entity_data, {"model", "model_path"}));
				std::string animation_bundle = AssetPathUtils::resolveModelPath(
					JsonUtils::readStringAlias(entity_data, {"animation_bundle"}));
				if (!model_path.empty())
					addModel(model_path);

				if (animation_bundle.empty() && !model_path.empty()) {
					animation_bundle = model_path;
				}

				if (!animation_bundle.empty())
					addAnimation(animation_bundle);
			} else {
				addModelAndAnimation("assets/models/actors/cat/cat_bounce.glb");
			}
			return;
		}

		if (entity_type == "mini_mushroom_prop") {
			addModelAndAnimation("assets/models/actors/mini_mushroom/mini_mushroom.glb");
			return;
		}

		if (entity_type == "static_object") {
			const std::string model_path = AssetPathUtils::resolveModelPath(
				JsonUtils::readStringAlias(entity_data, {"model", "model_path", "texture"}));
			if (!model_path.empty())
				addModel(model_path);
			return;
		}

		if (entity_type == "boss_trigger") {
			const std::string boss_id = entity_data.value("boss_id", "");
			if (boss_id == "bies" || boss_id == "devil" || boss_id.find("devil") != std::string::npos) {
				appendEntityTypeAssets("devil", entity_data);
				if (boss_id == "bies") {
					appendEntityTypeAssets("walking_dead", entity_data);
					appendEntityTypeAssets("bandit", entity_data);
				}
			} else if (boss_id == "czarownica" || boss_id == "witch" || boss_id.find("witch") != std::string::npos) {
				appendEntityTypeAssets("witch", entity_data);
			} else if (boss_id == "ropuch") {
				appendEntityTypeAssets("frog", entity_data);
			} else if (boss_id == "siewca_rozdarcia" || boss_id == "siewca_chaosu" ||
					   boss_id == "rift_binder" || boss_id == "dragon" ||
					   boss_id.find("rift") != std::string::npos || boss_id.find("chaos") != std::string::npos) {
				appendEntityTypeAssets("dragon", entity_data);
			}
		}

		if (entity_type == "story_trigger") {
			const std::string dialogue_key = entity_data.value("dialogue_key", "");
			if (dialogue_key == "witch_after_bies_placeholder")
				appendEntityTypeAssets("witch", entity_data);
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
