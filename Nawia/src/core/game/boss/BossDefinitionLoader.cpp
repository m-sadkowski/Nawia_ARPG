#include "BossDefinitionLoader.h"

#include <Logger.h>
#include <json.hpp>

#include <cmath>
#include <fstream>

namespace Nawia::Game {

	namespace {

		std::vector<MinionSpawnInfo> parseMinionList(const nlohmann::json& phase_json)
		{
			std::vector<MinionSpawnInfo> result;
			if (!phase_json.contains("minions"))
				return result;

			for (const auto& minion_json : phase_json["minions"]) {
				MinionSpawnInfo minion;
				minion.enemy_type = minion_json.value("enemy_type", "WalkingDead");
				minion.count = minion_json.value("count", 1);
				minion.hp = minion_json.value("hp", 60);
				minion.offset_x = minion_json.value("offset_x", 3.0f);
				minion.offset_y = minion_json.value("offset_y", 2.0f);
				result.push_back(minion);
			}
			return result;
		}

		BossPhase parseBossPhase(const nlohmann::json& phase_json)
		{
			BossPhase phase;
			phase.hp_threshold = phase_json.value("hp_threshold", 1.0f);
			phase.name = phase_json.value("name", "");
			phase.speed_multiplier = phase_json.value("speed_multiplier", 1.0f);
			phase.damage_multiplier = phase_json.value("damage_multiplier", 1.0f);
			phase.notification = phase_json.value("notification", "");
			phase.screen_flash = phase_json.value("screen_flash", false);

			if (phase_json.contains("flash_color")) {
				const auto& flash_color = phase_json["flash_color"];
				if (flash_color.is_array() && flash_color.size() >= 4) {
					phase.flash_color = {
						static_cast<unsigned char>(flash_color[0].get<int>()),
						static_cast<unsigned char>(flash_color[1].get<int>()),
						static_cast<unsigned char>(flash_color[2].get<int>()),
						static_cast<unsigned char>(flash_color[3].get<int>())
					};
				}
			}

			phase.minions = parseMinionList(phase_json);
			return phase;
		}

		BossReward parseBossReward(const nlohmann::json& reward_json)
		{
			BossReward reward;
			reward.gold = reward_json.value("gold", 0);
			reward.exp = reward_json.value("exp", 0);
			if (reward_json.contains("items")) {
				for (const auto& item_id : reward_json["items"])
					reward.item_ids.push_back(item_id.get<int>());
			}
			return reward;
		}

		BossIntroDialogue parseBossIntroDialogue(const nlohmann::json& boss_json)
		{
			BossIntroDialogue intro;
			if (!boss_json.contains("intro_dialogue") || !boss_json["intro_dialogue"].is_object())
				return intro;

			const auto& intro_json = boss_json["intro_dialogue"];
			intro.enabled = intro_json.value("enabled", false);
			intro.required_active_quest = intro_json.value("required_active_quest", "");
			intro.blocking_active_quest = intro_json.value("blocking_active_quest", "");
			intro.checkpoint_on_complete = intro_json.value("checkpoint_on_complete", "");
			intro.final_option = intro_json.value("final_option", "Rozumiem.");
			intro.show_preview = intro_json.value("show_preview", false);

			if (intro_json.contains("lines") && intro_json["lines"].is_array()) {
				for (const auto& line_json : intro_json["lines"]) {
					BossDialogueLine line;
					line.speaker = line_json.value("speaker", "");
					line.text = line_json.value("text", "");
					line.voice_path = line_json.value("voice_path", "");
					intro.lines.push_back(std::move(line));
				}
			}

			intro.enabled = intro.enabled && !intro.lines.empty();
			return intro;
		}

		BossPhase defaultPhase()
		{
			BossPhase phase;
			phase.hp_threshold = 1.0f;
			phase.name = "Faza 1";
			phase.speed_multiplier = 1.0f;
			phase.damage_multiplier = 1.0f;
			return phase;
		}

	} // namespace

	std::map<std::string, BossData> BossDefinitionLoader::loadFromJson(const std::string& path)
	{
		std::ifstream file(path);
		if (!file.is_open()) {
			Core::Logger::errorLog("BossManager: Nie udalo sie otworzyc " + path);
			return {};
		}

		nlohmann::json data;
		try {
			file >> data;
		} catch (const nlohmann::json::parse_error&) {
			Core::Logger::errorLog("BossManager: Blad parsowania JSON w " + path);
			return {};
		}

		if (!data.contains("bosses"))
			return {};

		std::map<std::string, BossData> bosses;
		for (const auto& boss_json : data["bosses"]) {
			BossData boss;
			boss.id = boss_json.value("id", "");
			boss.name = boss_json.value("name", "");
			boss.enemy_type = boss_json.value("enemy_type", "");
			boss.max_hp = boss_json.value("max_hp", 1000);
			boss.scale = boss_json.value("scale", 1.0f);
			boss.music_path = boss_json.value("music_path", "");
			boss.music_volume = boss_json.value("music_volume", 0.85f);
			boss.on_player_death = boss_json.value("on_player_death", "end_fight");
			boss.intro_dialogue = parseBossIntroDialogue(boss_json);
			boss.victory_dialogue_key = boss_json.value("victory_dialogue_key", "");
			boss.checkpoint_on_victory = boss_json.value("checkpoint_on_victory", "");

			if (boss_json.contains("phases")) {
				for (const auto& phase_json : boss_json["phases"])
					boss.phases.push_back(parseBossPhase(phase_json));
			}

			if (boss.phases.empty())
				boss.phases.push_back(defaultPhase());

			if (boss_json.contains("rewards"))
				boss.reward = parseBossReward(boss_json["rewards"]);

			if (!boss.id.empty()) {
				bosses[boss.id] = boss;
				Core::Logger::debugLog(
					"BossManager: Zaladowano bossa '" + boss.id + "' z " +
					std::to_string(boss.phases.size()) + " fazami");
			}
		}

		return bosses;
	}

} // namespace Nawia::Game
