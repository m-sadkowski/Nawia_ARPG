#pragma once

#include <raylib.h>

#include <string>
#include <vector>

namespace Nawia::Game {

	/**
	 * @brief Nagroda przyznawana graczowi po pokonaniu bossa.
	 */
	struct BossReward {
		std::vector<int> item_ids;
		int gold = 0;
		int exp = 0;
	};

	struct BossDialogueLine {
		std::string speaker;
		std::string text;
		std::string voice_path;
	};

	struct BossIntroDialogue {
		bool enabled = false;
		std::string required_active_quest;
		std::string blocking_active_quest;
		std::string checkpoint_on_complete;
		std::string final_option = "Rozumiem.";
		bool show_preview = false;
		std::vector<BossDialogueLine> lines;
	};

	/**
	 * @brief Opis pojedynczego typu miniona przywolywanego w fazie bossa.
	 */
	struct MinionSpawnInfo {
		std::string enemy_type;
		int count = 1;
		int hp = 60;
		float offset_x = 3.0f;
		float offset_y = 2.0f;
	};

	/**
	 * @brief Definicja pojedynczej fazy walki z bossem.
	 */
	struct BossPhase {
		float hp_threshold = 1.0f;
		std::string name;
		float speed_multiplier = 1.0f;
		float damage_multiplier = 1.0f;
		std::string notification;
		std::vector<MinionSpawnInfo> minions;
		bool screen_flash = false;
		Color flash_color = {255, 0, 0, 180};
	};

	/**
	 * @brief Pelna definicja bossa ladowana z JSON.
	 */
	struct BossData {
		std::string id;
		std::string name;
		std::string enemy_type;
		int max_hp = 1000;
		float scale = 1.0f;
		std::string music_path;
		float music_volume = 0.85f;

		std::vector<BossPhase> phases;
		BossReward reward;
		BossIntroDialogue intro_dialogue;
		std::string victory_dialogue_key;
		std::string checkpoint_on_victory;

		std::string on_player_death = "end_fight";
	};

	/**
	 * @brief Runtime'owy stan aktywnej walki, zapisywany per lokacja.
	 */
	struct BossRuntimeState {
		bool active = false;
		std::string boss_id;
		int current_phase_index = 0;
		float fight_timer = 0.0f;
		int saved_hp = 0;
		int max_hp = 0;
		Vector2 position = {0.0f, 0.0f};
		float altitude = 0.0f;
		Vector2 spawn_position = {0.0f, 0.0f};
		float spawn_altitude = 0.0f;
	};

} // namespace Nawia::Game
