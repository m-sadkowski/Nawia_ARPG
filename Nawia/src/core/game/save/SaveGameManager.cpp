#include "SaveGameManager.h"

#include <BossManager.h>
#include <Engine.h>
#include <Entity.h>
#include <EntityManager.h>
#include <Level.h>
#include <LevelManager.h>
#include <Logger.h>
#include <Player.h>
#include <QuestManager.h>
#include <SpawnManager.h>

#include <json.hpp>

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <utility>

using json = nlohmann::json;

namespace Nawia::Game {

	namespace {
		constexpr int SAVE_FORMAT_VERSION = 2;
		constexpr const char* SAVE_ROOT = "saves";
		constexpr int SAVE_SLOT_COUNT = 3;
		constexpr const char* SAVE_FILE_NAME = "save.json";

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

		json serializeAllies(const Nawia::Core::EntityManager& entity_manager) {
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

		void applyAllies(Nawia::Core::EntityManager& entity_manager, const json& allies_state) {
			if (!allies_state.is_array())
				return;

			// Mapujemy raz, zeby uniknac O(N*M) skanowania entity_manager dla kazdego sojusznika.
			std::unordered_map<std::string, std::shared_ptr<Nawia::Entity::Entity>> allies_by_name;
			for (const auto& entity : entity_manager.getEntities()) {
				if (entity && entity->getType() == Nawia::Entity::EntityType::Ally)
					allies_by_name.emplace(entity->getName(), entity);
			}

			for (const auto& ally_state : allies_state) {
				const std::string name = ally_state.value("name", "");
				if (name.empty())
					continue;

				const auto it = allies_by_name.find(name);
				if (it == allies_by_name.end())
					continue;

				const auto& entity = it->second;
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

		json serializeLocations(Nawia::Core::Engine& engine, const std::string& current_location) {
			json result = json::object();
			const auto current_level = engine.getLevelManager().getCurrentLevel();
			if (!current_level)
				return result;

			for (const auto& location : current_level->getLocations()) {
				json location_state;
				location_state["entities"] = current_level->getSpawnManager().serializeLocation(location);

				if (location == current_location) {
					if (const auto player = engine.getPlayer())
						location_state["player_view"] = player->serializeLocationView();

					if (engine.getBossManager().isFightActive())
						location_state["boss_fight"] = engine.getBossManager().serializeRuntimeState();
				}

				result[location] = std::move(location_state);
			}

			return result;
		}

		void applyLocations(Nawia::Core::Engine& engine, const json& locations_state) {
			if (!locations_state.is_object())
				return;

			auto current_level = engine.getLevelManager().getCurrentLevel();
			if (!current_level)
				return;

			const std::string current_location = engine.getLevelManager().getCurrentLocationName();

			for (const auto& [location_name, location_state] : locations_state.items()) {
				if (!location_state.is_object())
					continue;

				if (location_state.contains("entities"))
					current_level->getSpawnManager().applyLocation(location_name, location_state["entities"], engine.getItemDatabase());

				if (location_name != current_location)
					continue;

				if (location_state.contains("player_view")) {
					if (const auto player = engine.getPlayer())
						player->applyLocationView(location_state["player_view"]);
				}

				if (location_state.contains("boss_fight") && location_state["boss_fight"].is_object())
					engine.getBossManager().applyRuntimeState(location_state["boss_fight"], &engine);
			}
		}
	}

	std::filesystem::path SaveGameManager::getSaveRoot() {
		return SAVE_ROOT;
	}

	bool SaveGameManager::isValidSlot(const int slot) {
		return slot >= 1 && slot <= SAVE_SLOT_COUNT;
	}

	std::filesystem::path SaveGameManager::getSlotPath(const int slot) {
		return getSaveRoot() / ("slot_" + std::to_string(slot));
	}

	std::filesystem::path SaveGameManager::getSlotFilePath(const int slot) {
		return getSlotPath(slot) / SAVE_FILE_NAME;
	}

	std::vector<SaveSlotInfo> SaveGameManager::getSaveSlots() const {
		std::vector<SaveSlotInfo> slots;
		slots.reserve(SAVE_SLOT_COUNT);

		for (int slot = 1; slot <= SAVE_SLOT_COUNT; ++slot) {
			SaveSlotInfo info;
			info.slot = slot;

			json save_state;
			if (readJsonFile(getSlotFilePath(slot), save_state)) {
				info.occupied = true;
				info.saved_at = save_state.value("saved_at", "");
				info.current_level = save_state.value("current_level", "");
				info.current_location = save_state.value("current_location", "");
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
			const auto save_file = getSlotFilePath(slot);
			json save_state;
			if (!readJsonFile(save_file, save_state))
				continue;

			const long long saved_epoch = save_state.value("saved_at_epoch", 0LL);
			if (saved_epoch > 0) {
				if (saved_epoch > latest_epoch) {
					latest_epoch = saved_epoch;
					latest_slot = slot;
				}
				continue;
			}

			// Plik bez epoki: ustalamy najnowszy slot na podstawie czasu modyfikacji.
			const auto write_time = std::filesystem::last_write_time(save_file);
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

		json save_state;
		save_state["version"] = SAVE_FORMAT_VERSION;
		save_state["slot"] = slot;
		save_state["saved_at"] = makeTimestampText();
		save_state["saved_at_epoch"] = getUnixTimestamp();
		save_state["current_level"] = level_name;
		save_state["current_location"] = location_name;
		save_state["player"] = engine.getPlayer()->serializeProfile();
		save_state["allies"] = serializeAllies(engine.getEntityManager());
		save_state["quests"] = engine.getQuestManager().serializeState();
		save_state["defeated_bosses"] = engine.getBossManager().getDefeatedBossIds();
		save_state["locations"] = serializeLocations(engine, location_name);

		if (!writeJsonFile(getSlotFilePath(slot), save_state))
			return false;

		_active_slot = slot;
		return true;
	}

	bool SaveGameManager::tryReadSave(const int slot, json& out_data, int& resolved_slot) const {
		resolved_slot = slot == 0 ? findLatestSlot() : slot;
		if (!isValidSlot(resolved_slot))
			return false;

		return readJsonFile(getSlotFilePath(resolved_slot), out_data);
	}

	void SaveGameManager::applySaveState(Core::Engine& engine, const json& save_state, const int slot) {
		const std::string current_location_name = save_state.value("current_location", "");

		if (const auto player = engine.getPlayer()) {
			if (save_state.contains("player"))
				player->applyProfile(save_state["player"], engine.getItemDatabase());
		}

		if (!current_location_name.empty()) {
			if (auto current_level = engine.getLevelManager().getCurrentLevel()) {
				if (current_level->getCurrentLocationName() != current_location_name)
					current_level->changeLocation(&engine, current_location_name);
			}
		}

		if (save_state.contains("allies"))
			applyAllies(engine.getEntityManager(), save_state["allies"]);

		if (save_state.contains("quests"))
			engine.getQuestManager().applyState(save_state["quests"]);

		if (save_state.contains("defeated_bosses") && save_state["defeated_bosses"].is_array()) {
			std::vector<std::string> defeated_bosses;
			for (const auto& boss_id : save_state["defeated_bosses"]) {
				if (boss_id.is_string())
					defeated_bosses.push_back(boss_id.get<std::string>());
			}
			engine.getBossManager().setDefeatedBossIds(defeated_bosses);
		}

		if (save_state.contains("locations"))
			applyLocations(engine, save_state["locations"]);

		_active_slot = slot;
	}

	bool SaveGameManager::loadGame(Core::Engine& engine, const int slot) {
		json save_state;
		int resolved_slot = 0;
		if (!tryReadSave(slot, save_state, resolved_slot))
			return false;

		const std::string current_level_name = save_state.value("current_level", "");
		if (current_level_name.empty())
			return false;

		engine.getBossManager().resetRuntimeState(&engine);
		engine.getBossManager().clearDefeatedBosses();
		engine.getQuestManager().resetAll();

		engine.getLevelManager().changeLevel(current_level_name, &engine);
		applySaveState(engine, save_state, resolved_slot);
		return true;
	}

} // namespace Nawia::Game
