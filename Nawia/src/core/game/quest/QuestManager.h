#pragma once

#include "Quest.h"

#include <map>
#include <string>
#include <vector>
#include <functional>

namespace Nawia::Core { class Engine; }

namespace Nawia::Game {

	/**
	 * @class QuestManager
	 * @brief Manages all quests: loading from JSON, tracking progress, unlocking chains.
	 */
	class QuestManager {
	public:
		/// Load quest definitions from a JSON file
		void loadFromJson(const std::string& path);

		/// Reset all quests to initial state (call on level enter)
		void resetAll();

		/// Start a quest by ID (changes Available -> Active)
		bool startQuest(const std::string& id);

		/// Force-complete a quest and give rewards
		void completeQuest(const std::string& id, Core::Engine* engine);

		/// Get quest by ID (nullptr if not found)
		[[nodiscard]] Quest* getQuest(const std::string& id);
		[[nodiscard]] const Quest* getQuest(const std::string& id) const;

		/// Set the current level name (called on level change)
		void setCurrentLevel(const std::string& level_name);

		/// Get quests filtered by state (only for current level)
		[[nodiscard]] std::vector<Quest*> getActiveQuests();
		[[nodiscard]] std::vector<Quest*> getAvailableQuests();
		[[nodiscard]] std::vector<Quest*> getCompletedQuests();

		/// Get all quests assigned to a specific level
		[[nodiscard]] std::vector<Quest*> getQuestsForLevel(const std::string& level_name);

		// --- Notification methods (call these when game events happen) ---
		void notifyKill(const std::string& enemy_name);
		void notifyItemCollected(int item_id);
		void notifyItemDelivered(int item_id, const std::string& npc_name);
		void notifyCheckpointReached(const std::string& checkpoint_name);
		void notifyNPCTalked(const std::string& npc_name);

		/// Check quest states: unlock available, auto-complete finished quests
		void update(Core::Engine* engine);

	private:
		/// Check if all prerequisites of a quest are completed
		[[nodiscard]] bool arePrerequisitesMet(const Quest& quest, Core::Engine* engine) const;

		/// Check if a quest belongs to the current level (or is global)
		[[nodiscard]] bool isQuestForCurrentLevel(const Quest& quest) const;

		std::map<std::string, Quest> _quests;
		std::string _current_level;
	};

} // namespace Nawia::Game
