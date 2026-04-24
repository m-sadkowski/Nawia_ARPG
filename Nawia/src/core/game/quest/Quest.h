#pragma once

#include <string>
#include <vector>

namespace Nawia::Game {

	enum class QuestState {
		Locked,		///< Prerequisites not met
		Available,	///< Prerequisites met, waiting for player to accept
		Active,		///< Player accepted, tracking objectives
		Completed,	///< All objectives done, reward given
		Failed		///< Quest failed (optional future use)
	};

	enum class ObjectiveType {
		Kill,				///< Kill N enemies of given name
		CollectItem,		///< Have N items of given ID in backpack
		DeliverItem,		///< Give item to NPC
		ReachCheckpoint,	///< Reach a named checkpoint
		TalkToNPC			///< Talk to a named NPC
	};

	struct QuestObjective {
		ObjectiveType type;
		std::string description;
		std::string target_name;	///< enemy name / NPC name / checkpoint name
		int item_id = 0;			///< for CollectItem / DeliverItem
		int required_count = 1;
		int current_count = 0;

		[[nodiscard]] bool isCompleted() const { return current_count >= required_count; }

		void progress(const int amount = 1) {
			current_count += amount;
			if (current_count > required_count)
				current_count = required_count;
		}

		void reset() { current_count = 0; }
	};

	struct QuestReward {
		std::vector<int> item_ids;
		int gold = 0;
		int exp = 0;
	};

	class Quest {
	public:
		std::string id;
		std::string name;
		std::string description;
		QuestState state = QuestState::Locked;
		bool auto_start = false;
		int required_level = 1;

		std::vector<std::string> prerequisites;	///< IDs of quests that must be Completed first
		std::vector<QuestObjective> objectives;
		QuestReward reward;

		[[nodiscard]] bool areAllObjectivesComplete() const {
			for (const auto& obj : objectives) {
				if (!obj.isCompleted()) return false;
			}
			return true;
		}

		[[nodiscard]] bool isCompleted() const { return state == QuestState::Completed; }
		[[nodiscard]] bool isActive() const { return state == QuestState::Active; }
		[[nodiscard]] bool isAvailable() const { return state == QuestState::Available; }
		[[nodiscard]] bool isLocked() const { return state == QuestState::Locked; }

		void start() {
			if (state == QuestState::Available)
				state = QuestState::Active;
		}

		void complete() {
			state = QuestState::Completed;
		}

		void reset() {
			state = QuestState::Locked;
			for (auto& obj : objectives)
				obj.reset();
		}

		/// Returns progress string like "1/3"
		[[nodiscard]] std::string getProgressString() const {
			int done = 0, total = 0;
			for (const auto& obj : objectives) {
				done += obj.current_count;
				total += obj.required_count;
			}
			return std::to_string(done) + "/" + std::to_string(total);
		}
	};

} // namespace Nawia::Game
