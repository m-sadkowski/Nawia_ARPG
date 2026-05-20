#include "SaveGameManager.h"

#include <Backpack.h>
#include <BossManager.h>
#include <Cat.h>
#include <Checkpoint.h>
#include <Chest.h>
#include <Engine.h>
#include <Entity.h>
#include <EntityManager.h>
#include <Equipment.h>
#include <ItemDatabase.h>
#include <Level.h>
#include <LevelManager.h>
#include <Logger.h>
#include <Player.h>
#include <QuestManager.h>
#include <SpawnManager.h>

#include <json.hpp>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <utility>

using json = nlohmann::json;

namespace Nawia::Game {

	namespace {
		constexpr int SAVE_FORMAT_VERSION = 1;
		constexpr const char* SAVE_ROOT = "saves";
		constexpr int SAVE_SLOT_COUNT = 3;

		std::string makeTimestampText() {
			const auto now = std::chrono::system_clock::now();
			const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
			std::tm local_time{};

#if defined(_WIN32)
			localtime_s(&local_time, &now_time);
#else
			localtime_r(&now_time, &local_time);
#endif

			std::ostringstream stream;
			stream << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
			return stream.str();
		}

		long long getUnixTimestamp() {
			const auto now = std::chrono::system_clock::now();
			return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
		}

		std::string sanitizeFileName(const std::string& text) {
			std::string result;
			result.reserve(text.size());

			for (const unsigned char character : text) {
				if ((character >= 'a' && character <= 'z') ||
					(character >= 'A' && character <= 'Z') ||
					(character >= '0' && character <= '9')) {
					result.push_back(static_cast<char>(character));
				} else {
					result.push_back('_');
				}
			}

			while (result.find("__") != std::string::npos)
				result.replace(result.find("__"), 2, "_");

			if (!result.empty() && result.front() == '_')
				result.erase(result.begin());
			if (!result.empty() && result.back() == '_')
				result.pop_back();

			return result.empty() ? "state" : result;
		}

		json vector2ToJson(const Vector2 value) {
			return {{"x", value.x}, {"y", value.y}};
		}

		json vector3ToJson(const Vector3 value) {
			return {{"x", value.x}, {"y", value.y}, {"z", value.z}};
		}

		Vector2 jsonToVector2(const json& data, const Vector2 fallback = {0.0f, 0.0f}) {
			if (!data.is_object())
				return fallback;

			return {
				data.value("x", fallback.x),
				data.value("y", fallback.y)
			};
		}

		Nawia::Entity::Stats statsFromJson(const json& data, const Nawia::Entity::Stats& fallback) {
			if (!data.is_object())
				return fallback;

			Nawia::Entity::Stats stats = fallback;
			stats.max_hp = data.value("max_hp", stats.max_hp);
			stats.damage = data.value("damage", stats.damage);
			stats.power = data.value("power", stats.power);
			stats.attack_speed = data.value("attack_speed", stats.attack_speed);
			stats.movement_speed = data.value("movement_speed", stats.movement_speed);
			stats.defense = data.value("defense", stats.defense);
			return stats;
		}

		json statsToJson(const Nawia::Entity::Stats& stats) {
			return {
				{"max_hp", stats.max_hp},
				{"damage", stats.damage},
				{"power", stats.power},
				{"attack_speed", stats.attack_speed},
				{"movement_speed", stats.movement_speed},
				{"defense", stats.defense}
			};
		}

		std::string equipmentSlotToString(const Nawia::Item::EquipmentSlot slot) {
			switch (slot) {
				case Nawia::Item::EquipmentSlot::Head: return "Head";
				case Nawia::Item::EquipmentSlot::Neck: return "Neck";
				case Nawia::Item::EquipmentSlot::Chest: return "Chest";
				case Nawia::Item::EquipmentSlot::Legs: return "Legs";
				case Nawia::Item::EquipmentSlot::Feet: return "Feet";
				case Nawia::Item::EquipmentSlot::Weapon: return "Weapon";
				case Nawia::Item::EquipmentSlot::OffHand: return "OffHand";
				case Nawia::Item::EquipmentSlot::Ring: return "Ring";
				case Nawia::Item::EquipmentSlot::None: return "None";
			}

			return "None";
		}

		Nawia::Item::EquipmentSlot equipmentSlotFromString(const std::string& slot) {
			if (slot == "Head") return Nawia::Item::EquipmentSlot::Head;
			if (slot == "Neck") return Nawia::Item::EquipmentSlot::Neck;
			if (slot == "Chest") return Nawia::Item::EquipmentSlot::Chest;
			if (slot == "Legs") return Nawia::Item::EquipmentSlot::Legs;
			if (slot == "Feet") return Nawia::Item::EquipmentSlot::Feet;
			if (slot == "Weapon") return Nawia::Item::EquipmentSlot::Weapon;
			if (slot == "OffHand") return Nawia::Item::EquipmentSlot::OffHand;
			if (slot == "Ring") return Nawia::Item::EquipmentSlot::Ring;
			return Nawia::Item::EquipmentSlot::None;
		}

		json backpackToJson(const Nawia::Item::Backpack& backpack) {
			json result;
			result["capacity"] = backpack.getCapacity();
			result["items"] = json::array();

			const auto& items = backpack.getItems();
			for (size_t i = 0; i < items.size(); ++i) {
				if (!items[i])
					continue;

				result["items"].push_back({
					{"slot", i},
					{"item_id", items[i]->getId()}
				});
			}

			return result;
		}

		void applyBackpackState(
			Nawia::Item::Backpack& backpack,
			const json& data,
			Nawia::Item::ItemDatabase& item_database
		) {
			backpack.clear();

			if (!data.contains("items") || !data["items"].is_array())
				return;

			for (const auto& item_state : data["items"]) {
				const int slot = item_state.value("slot", -1);
				const int item_id = item_state.value("item_id", 0);
				if (slot < 0 || item_id <= 0)
					continue;

				if (auto item = item_database.createItem(item_id))
					backpack.setItem(slot, item);
			}
		}

		json equipmentToJson(const Nawia::Item::Equipment& equipment) {
			json result = json::array();

			for (const auto& [slot, item] : equipment.getSlots()) {
				if (!item)
					continue;

				result.push_back({
					{"slot", equipmentSlotToString(slot)},
					{"item_id", item->getId()}
				});
			}

			return result;
		}

		void applyEquipmentState(
			Nawia::Entity::Player& player,
			const json& data,
			Nawia::Item::ItemDatabase& item_database
		) {
			player.getEquipment().clear();

			if (!data.is_array())
				return;

			for (const auto& item_state : data) {
				const auto slot = equipmentSlotFromString(item_state.value("slot", "None"));
				const int item_id = item_state.value("item_id", 0);
				if (slot == Nawia::Item::EquipmentSlot::None || item_id <= 0)
					continue;

				if (auto item = item_database.createItem(item_id))
					player.equipItem(item);
			}
		}

		std::string entityTypeToString(const Nawia::Entity::EntityType type) {
			switch (type) {
				case Nawia::Entity::EntityType::Player: return "player";
				case Nawia::Entity::EntityType::Enemy: return "enemy";
				case Nawia::Entity::EntityType::Ally: return "ally";
				case Nawia::Entity::EntityType::NPCStatic: return "npc";
				case Nawia::Entity::EntityType::Projectile: return "projectile";
				case Nawia::Entity::EntityType::Trigger: return "trigger";
				case Nawia::Entity::EntityType::Chest: return "chest";
				case Nawia::Entity::EntityType::Item: return "item";
				case Nawia::Entity::EntityType::None: return "none";
			}

			return "none";
		}

		json entityBaseToJson(const Nawia::Entity::Entity& entity) {
			return {
				{"name", entity.getName()},
				{"type", entityTypeToString(entity.getType())},
				{"position", vector2ToJson({entity.getX(), entity.getY()})},
				{"altitude", entity.getAltitude()},
				{"rotation", entity.getRotation()},
				{"hp", entity.getHP()},
				{"max_hp", entity.getMaxHP()},
				{"dead", entity.isDead()},
				{"dormant", entity.isDormant()}
			};
		}

		json bossRuntimeStateToJson(const Nawia::Game::BossRuntimeState& state) {
			json result;
			result["active"] = state.active;
			if (!state.active)
				return result;

			result["boss_id"] = state.boss_id;
			result["current_phase_index"] = state.current_phase_index;
			result["fight_timer"] = state.fight_timer;
			result["saved_hp"] = state.saved_hp;
			result["max_hp"] = state.max_hp;
			result["position"] = vector2ToJson(state.position);
			result["altitude"] = state.altitude;
			result["spawn_position"] = vector2ToJson(state.spawn_position);
			result["spawn_altitude"] = state.spawn_altitude;
			return result;
		}

		Nawia::Game::BossRuntimeState bossRuntimeStateFromJson(const json& data) {
			Nawia::Game::BossRuntimeState state;
			if (!data.is_object())
				return state;

			state.active = data.value("active", false);
			state.boss_id = data.value("boss_id", "");
			state.current_phase_index = data.value("current_phase_index", 0);
			state.fight_timer = data.value("fight_timer", 0.0f);
			state.saved_hp = data.value("saved_hp", 0);
			state.max_hp = data.value("max_hp", 0);
			state.position = jsonToVector2(data.value("position", json::object()));
			state.altitude = data.value("altitude", 0.0f);
			state.spawn_position = jsonToVector2(data.value("spawn_position", json::object()));
			state.spawn_altitude = data.value("spawn_altitude", 0.0f);
			return state;
		}

		void applyBaseEntityState(Nawia::Entity::Entity& entity, const json& data) {
			if (!data.is_object())
				return;

			const Vector2 position = jsonToVector2(data.value("position", json::object()), {entity.getX(), entity.getY()});
			entity.setX(position.x);
			entity.setY(position.y);
			entity.setAltitude(data.value("altitude", entity.getAltitude()));
			entity.setRotation(data.value("rotation", entity.getRotation()));

			if (data.contains("max_hp") && data["max_hp"].is_number_integer())
				entity.setMaxHp(data["max_hp"].get<int>());

			const int hp = data.value("hp", entity.getHP());
			if (data.value("dead", false) || hp <= 0) {
				entity.die();
				entity.setDormant(true);
			} else {
				entity.setHP(hp);
				entity.setDormant(data.value("dormant", entity.isDormant()));
			}
		}

		std::string makeSpawnStableId(const Nawia::World::SpawnPoint& spawn_point, const size_t index) {
			return std::to_string(index) + "|" +
				spawn_point.location + "|" +
				spawn_point.entity_type + "|" +
				spawn_point.entity_data.value("name", "");
		}

		json spawnPointToJson(const Nawia::World::SpawnPoint& spawn_point, const size_t index) {
			json result;
			result["spawn_index"] = index;
			result["stable_id"] = makeSpawnStableId(spawn_point, index);
			result["location"] = spawn_point.location;
			result["entity_type"] = spawn_point.entity_type;
			result["activated"] = spawn_point.activated;
			result["spawn_center"] = vector2ToJson(spawn_point.spawn_center);
			result["trigger_radius"] = spawn_point.trigger_radius;
			result["spawn_radius"] = spawn_point.spawn_radius;

			if (!spawn_point.entity)
				return result;

			result["entity"] = entityBaseToJson(*spawn_point.entity);

			if (const auto chest = std::dynamic_pointer_cast<Nawia::Entity::Chest>(spawn_point.entity)) {
				result["chest"] = {
					{"open", chest->isOpen()},
					{"locked", chest->isLocked()},
					{"key_id", chest->getKeyId()},
					{"inventory", backpackToJson(*chest->getStoredInventory())}
				};
			} else if (const auto cat = std::dynamic_pointer_cast<Nawia::Entity::Cat>(spawn_point.entity)) {
				result["npc"] = {
					{"open", cat->isOpen()},
					{"quest_completed", cat->isQuestCompleted()},
					{"inventory", backpackToJson(*cat->getInventory())}
				};
			} else if (const auto checkpoint = std::dynamic_pointer_cast<Nawia::Entity::Checkpoint>(spawn_point.entity)) {
				result["checkpoint"] = {
					{"activated", checkpoint->isActivated()}
				};
			}

			return result;
		}

		void applySpawnPointState(
			Nawia::World::SpawnPoint& spawn_point,
			const json& state,
			Nawia::Item::ItemDatabase& item_database
		) {
			spawn_point.activated = state.value("activated", spawn_point.activated);

			if (!spawn_point.entity)
				return;

			if (state.contains("entity"))
				applyBaseEntityState(*spawn_point.entity, state["entity"]);

			if (const auto chest = std::dynamic_pointer_cast<Nawia::Entity::Chest>(spawn_point.entity)) {
				const json chest_state = state.value("chest", json::object());
				chest->setOpen(chest_state.value("open", chest->isOpen()));
				chest->setLocked(chest_state.value("locked", chest->isLocked()), chest_state.value("key_id", chest->getKeyId()));
				if (chest_state.contains("inventory") && chest->getStoredInventory())
					applyBackpackState(*chest->getStoredInventory(), chest_state["inventory"], item_database);
			} else if (const auto cat = std::dynamic_pointer_cast<Nawia::Entity::Cat>(spawn_point.entity)) {
				const json npc_state = state.value("npc", json::object());
				cat->setOpen(npc_state.value("open", cat->isOpen()));
				cat->setQuestCompleted(npc_state.value("quest_completed", cat->isQuestCompleted()));
				if (npc_state.contains("inventory") && cat->getInventory())
					applyBackpackState(*cat->getInventory(), npc_state["inventory"], item_database);
			} else if (const auto checkpoint = std::dynamic_pointer_cast<Nawia::Entity::Checkpoint>(spawn_point.entity)) {
				const json checkpoint_state = state.value("checkpoint", json::object());
				checkpoint->setActivated(checkpoint_state.value("activated", checkpoint->isActivated()));
			}

			if (spawn_point.entity->isDead()) {
				spawn_point.activated = true;
				spawn_point.entity->setDormant(true);
			}
		}

		json playerGlobalToJson(const Nawia::Entity::Player& player) {
			return {
				{"level", player.getLevel()},
				{"exp", player.getExp()},
				{"exp_to_next_level", player.getExpToNextLvl()},
				{"gold", player.getGold()},
				{"base_stats", statsToJson(player.getBaseStats())},
				{"inventory", backpackToJson(player.getBackpack())},
				{"equipment", equipmentToJson(player.getEquipment())},
				{"knowledge", json::object()}
			};
		}

		void applyPlayerGlobalState(
			Nawia::Entity::Player& player,
			const json& data,
			Nawia::Item::ItemDatabase& item_database
		) {
			player.clearItems();
			player.setLevel(data.value("level", player.getLevel()));
			player.setExp(data.value("exp", player.getExp()));
			player.setExpToNextLvl(data.value("exp_to_next_level", player.getExpToNextLvl()));
			player.setGold(data.value("gold", player.getGold()));
			player.setBaseStats(statsFromJson(data.value("base_stats", json::object()), player.getBaseStats()));

			if (data.contains("inventory"))
				applyBackpackState(player.getBackpack(), data["inventory"], item_database);
			if (data.contains("equipment"))
				applyEquipmentState(player, data["equipment"], item_database);

			player.recalculateStats();
		}

		json playerLocationToJson(const Nawia::Entity::Player& player) {
			return {
				{"position", vector2ToJson({player.getX(), player.getY()})},
				{"altitude", player.getAltitude()},
				{"hp", player.getHP()},
				{"max_hp", player.getMaxHP()},
				{"respawn_point", vector2ToJson(player.getRespawnPoint())}
			};
		}

		void applyPlayerLocationState(Nawia::Entity::Player& player, const json& data) {
			if (!data.is_object())
				return;

			const Vector2 position = jsonToVector2(data.value("position", json::object()), {player.getX(), player.getY()});
			player.setX(position.x);
			player.setY(position.y);
			player.setAltitude(data.value("altitude", player.getAltitude()));
			player.setRespawnPoint(jsonToVector2(data.value("respawn_point", json::object()), player.getRespawnPoint()));
			player.setHP(data.value("hp", player.getHP()));
			player.stop();
		}

		json alliesGlobalToJson(const Nawia::Core::EntityManager& entity_manager) {
			json result = json::array();

			for (const auto& entity : entity_manager.getEntities()) {
				if (!entity || entity->getType() != Nawia::Entity::EntityType::Ally)
					continue;

				result.push_back({
					{"name", entity->getName()},
					{"hp", entity->getHP()},
					{"max_hp", entity->getMaxHP()},
					{"dead", entity->isDead()}
				});
			}

			return result;
		}

		void applyAlliesGlobalState(Nawia::Core::EntityManager& entity_manager, const json& allies_state) {
			if (!allies_state.is_array())
				return;

			std::map<std::string, json> allies_by_name;
			for (const auto& ally_state : allies_state) {
				const std::string name = ally_state.value("name", "");
				if (!name.empty())
					allies_by_name[name] = ally_state;
			}

			for (const auto& entity : entity_manager.getEntities()) {
				if (!entity || entity->getType() != Nawia::Entity::EntityType::Ally)
					continue;

				const auto ally_it = allies_by_name.find(entity->getName());
				if (ally_it == allies_by_name.end())
					continue;

				const json& ally_state = ally_it->second;
				if (ally_state.contains("max_hp"))
					entity->setMaxHp(ally_state.value("max_hp", entity->getMaxHP()));

				if (ally_state.value("dead", false)) {
					entity->die();
					entity->setDormant(true);
				} else {
					entity->setHP(ally_state.value("hp", entity->getHP()));
				}
			}
		}

		json locationStateToJson(
			Nawia::Core::Engine& engine,
			const std::string& level_name,
			const std::string& location_name
		) {
			json state;
			state["version"] = SAVE_FORMAT_VERSION;
			state["level"] = level_name;
			state["location"] = location_name;
			state["entities"] = json::array();

			const auto current_level = engine.getLevelManager().getCurrentLevel();
			if (!current_level)
				return state;

			const auto& spawn_points = current_level->getSpawnManager().getSpawnPoints();
			for (size_t i = 0; i < spawn_points.size(); ++i) {
				if (spawn_points[i].location == location_name)
					state["entities"].push_back(spawnPointToJson(spawn_points[i], i));
			}

			if (engine.getLevelManager().getCurrentLocationName() == location_name) {
				if (const auto player = engine.getPlayer())
					state["player"] = playerLocationToJson(*player);

				const auto boss_state = engine.getBossManager().getRuntimeState();
				if (boss_state.active)
					state["boss_fight"] = bossRuntimeStateToJson(boss_state);
			}

			return state;
		}

		bool writeJsonFile(const std::filesystem::path& path, const json& data) {
			std::filesystem::create_directories(path.parent_path());

			std::ofstream file(path);
			if (!file.is_open()) {
				Nawia::Core::Logger::errorLog("SaveGame: nie mozna zapisac " + path.generic_string());
				return false;
			}

			file << std::setw(2) << data;
			return true;
		}

		bool readJsonFile(const std::filesystem::path& path, json& data) {
			std::ifstream file(path);
			if (!file.is_open())
				return false;

			try {
				file >> data;
			} catch (const json::parse_error& error) {
				Nawia::Core::Logger::errorLog("SaveGame: blad parsowania " + path.generic_string() + ": " + error.what());
				return false;
			}

			return true;
		}
	}

	std::filesystem::path SaveGameManager::getSaveRoot() const {
		return SAVE_ROOT;
	}

	bool SaveGameManager::isValidSlot(const int slot) const {
		return slot >= 1 && slot <= SAVE_SLOT_COUNT;
	}

	std::filesystem::path SaveGameManager::getSlotPath(const int slot) const {
		return getSaveRoot() / ("slot_" + std::to_string(slot));
	}

	std::vector<SaveSlotInfo> SaveGameManager::getSaveSlots() const {
		std::vector<SaveSlotInfo> slots;
		slots.reserve(SAVE_SLOT_COUNT);

		for (int slot = 1; slot <= SAVE_SLOT_COUNT; ++slot) {
			SaveSlotInfo info;
			info.slot = slot;

			json profile_state;
			if (readJsonFile(getSlotPath(slot) / "profile.json", profile_state)) {
				info.occupied = true;
				info.saved_at = profile_state.value("saved_at", "");
				info.current_level = profile_state.value("current_level", "");
				info.current_location = profile_state.value("current_location", "");
			}

			slots.push_back(std::move(info));
		}

		return slots;
	}

	bool SaveGameManager::hasAnySave() const {
		return findLatestSlot() != 0;
	}

	int SaveGameManager::findLatestSlot() const {
		int latest_slot = 0;
		long long latest_epoch = 0;
		std::filesystem::file_time_type latest_time{};
		bool has_file_time = false;

		for (int slot = 1; slot <= SAVE_SLOT_COUNT; ++slot) {
			const auto profile_file = getSlotPath(slot) / "profile.json";
			json profile_state;
			if (!readJsonFile(profile_file, profile_state))
				continue;

			const long long saved_epoch = profile_state.value("saved_at_epoch", 0LL);
			if (saved_epoch > 0) {
				if (saved_epoch > latest_epoch) {
					latest_epoch = saved_epoch;
					latest_slot = slot;
				}
				continue;
			}

			const auto write_time = std::filesystem::last_write_time(profile_file);
			if (!has_file_time || write_time > latest_time) {
				has_file_time = true;
				latest_time = write_time;
				latest_slot = slot;
			}
		}

		return latest_slot;
	}

	bool SaveGameManager::saveGame(Core::Engine& engine, const int slot) {
		if (!isValidSlot(slot))
			return false;

		const std::string level_name = engine.getLevelManager().getCurrentLevelName();
		const std::string location_name = engine.getLevelManager().getCurrentLocationName();
		if (level_name.empty() || location_name.empty() || !engine.getPlayer())
			return false;

		const std::filesystem::path profile_path = getSlotPath(slot);
		const std::filesystem::path levels_dir = profile_path / "levels";
		const std::filesystem::path locations_dir = profile_path / "locations";
		std::filesystem::create_directories(levels_dir);
		std::filesystem::create_directories(locations_dir);

		json location_files = json::object();
		const auto current_level = engine.getLevelManager().getCurrentLevel();
		if (current_level) {
			for (const auto& location : current_level->getLocations()) {
				const std::string file_name = sanitizeFileName(level_name) + "__" + sanitizeFileName(location) + ".json";
				const std::filesystem::path location_path = locations_dir / file_name;
				location_files[location] = (std::filesystem::path("locations") / file_name).generic_string();
				writeJsonFile(location_path, locationStateToJson(engine, level_name, location));
			}
		}

		const std::string level_file_name = sanitizeFileName(level_name) + ".json";
		json level_state;
		level_state["version"] = SAVE_FORMAT_VERSION;
		level_state["level"] = level_name;
		level_state["completed"] = false;
		level_state["quests"] = engine.getQuestManager().serializeState();
		level_state["defeated_bosses"] = engine.getBossManager().getDefeatedBossIds();
		level_state["locations"] = location_files;
		writeJsonFile(levels_dir / level_file_name, level_state);

		json player_state = playerGlobalToJson(*engine.getPlayer());
		player_state["allies"] = alliesGlobalToJson(engine.getEntityManager());
		writeJsonFile(profile_path / "player.json", player_state);

		json profile_state = json::object();
		readJsonFile(profile_path / "profile.json", profile_state);
		profile_state["version"] = SAVE_FORMAT_VERSION;
		profile_state["profile_id"] = "slot_" + std::to_string(slot);
		profile_state["slot"] = slot;
		profile_state["saved_at"] = makeTimestampText();
		profile_state["saved_at_epoch"] = getUnixTimestamp();
		profile_state["current_level"] = level_name;
		profile_state["current_location"] = location_name;
		profile_state["player_file"] = "player.json";
		if (!profile_state.contains("levels") || !profile_state["levels"].is_object())
			profile_state["levels"] = json::object();
		profile_state["levels"][level_name] = (std::filesystem::path("levels") / level_file_name).generic_string();
		if (!profile_state.contains("knowledge") || !profile_state["knowledge"].is_object())
			profile_state["knowledge"] = json::object();

		const bool saved_profile = writeJsonFile(profile_path / "profile.json", profile_state);
		if (saved_profile)
			_active_slot = slot;

		return saved_profile;
	}

	bool SaveGameManager::loadLatestGame(Core::Engine& engine) {
		return loadGame(engine, findLatestSlot());
	}

	bool SaveGameManager::loadGame(Core::Engine& engine, const int slot) {
		if (!isValidSlot(slot))
			return false;

		const std::filesystem::path profile_path = getSlotPath(slot);

		json profile_state;
		if (!readJsonFile(profile_path / "profile.json", profile_state))
			return false;

		const std::string current_level_name = profile_state.value("current_level", "");
		const std::string current_location_name = profile_state.value("current_location", "");
		if (current_level_name.empty())
			return false;

		engine.getBossManager().resetRuntimeState(&engine);
		engine.getBossManager().clearDefeatedBosses();
		engine.getQuestManager().resetAll();

		json player_state;
		readJsonFile(profile_path / profile_state.value("player_file", "player.json"), player_state);
		if (const auto player = engine.getPlayer()) {
			if (!player_state.empty())
				applyPlayerGlobalState(*player, player_state, engine.getItemDatabase());
		}

		engine.getLevelManager().changeLevel(current_level_name, &engine);
		if (!current_location_name.empty()) {
			if (auto current_level = engine.getLevelManager().getCurrentLevel()) {
				if (current_level->getCurrentLocationName() != current_location_name)
					current_level->changeLocation(&engine, current_location_name);
			}
		}

		if (player_state.contains("allies"))
			applyAlliesGlobalState(engine.getEntityManager(), player_state["allies"]);

		std::string level_file;
		if (profile_state.contains("levels") && profile_state["levels"].is_object())
			level_file = profile_state["levels"].value(current_level_name, "");
		json level_state;
		if (!level_file.empty() && readJsonFile(profile_path / level_file, level_state)) {
			if (level_state.contains("quests"))
				engine.getQuestManager().applyState(level_state["quests"]);

			if (level_state.contains("defeated_bosses") && level_state["defeated_bosses"].is_array()) {
				std::vector<std::string> defeated_bosses;
				for (const auto& boss_id : level_state["defeated_bosses"]) {
					if (boss_id.is_string())
						defeated_bosses.push_back(boss_id.get<std::string>());
				}
				engine.getBossManager().setDefeatedBossIds(defeated_bosses);
			}

			if (level_state.contains("locations") && level_state["locations"].is_object()) {
				auto current_level = engine.getLevelManager().getCurrentLevel();
				json current_location_boss_state;
				bool has_current_location_boss_state = false;

				for (const auto& [location_name, location_file_json] : level_state["locations"].items()) {
					if (!current_level || !location_file_json.is_string())
						continue;

					json location_state;
					if (!readJsonFile(profile_path / location_file_json.get<std::string>(), location_state))
						continue;

					auto& spawn_points = current_level->getSpawnManager().getSpawnPoints();
					std::map<std::string, size_t> stable_ids;
					for (size_t i = 0; i < spawn_points.size(); ++i)
						stable_ids[makeSpawnStableId(spawn_points[i], i)] = i;

					if (location_state.contains("entities") && location_state["entities"].is_array()) {
						for (const auto& spawn_state : location_state["entities"]) {
							size_t spawn_index = static_cast<size_t>(spawn_state.value("spawn_index", -1));
							if (spawn_index >= spawn_points.size()) {
								const std::string stable_id = spawn_state.value("stable_id", "");
								const auto stable_it = stable_ids.find(stable_id);
								if (stable_it == stable_ids.end())
									continue;

								spawn_index = stable_it->second;
							}

							applySpawnPointState(spawn_points[spawn_index], spawn_state, engine.getItemDatabase());
						}
					}

					if (location_name == engine.getLevelManager().getCurrentLocationName() &&
						location_state.contains("player") &&
						engine.getPlayer()) {
						applyPlayerLocationState(*engine.getPlayer(), location_state["player"]);
					}

					if (location_name == engine.getLevelManager().getCurrentLocationName() &&
						location_state.contains("boss_fight") &&
						location_state["boss_fight"].is_object()) {
						current_location_boss_state = location_state["boss_fight"];
						has_current_location_boss_state = current_location_boss_state.value("active", false);
					}
				}

				if (has_current_location_boss_state) {
					const auto boss_state = bossRuntimeStateFromJson(current_location_boss_state);
					engine.getBossManager().restoreRuntimeState(boss_state, &engine);
				}
			}
		}

		_active_slot = slot;
		return true;
	}

} // namespace Nawia::Game
