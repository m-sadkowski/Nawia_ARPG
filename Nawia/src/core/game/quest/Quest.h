#pragma once

#include <string>
#include <vector>

namespace Nawia::Game {

	/**
	 * @enum QuestState
	 * @brief Opisuje aktualny stan questa.
	 */
	enum class QuestState {
		Locked,    ///< Wymagania nie sa jeszcze spelnione.
		Available, ///< Quest jest gotowy do przyjecia.
		Active,    ///< Quest jest aktywny i sledzi cele.
		Completed, ///< Cele zostaly wykonane i nagroda zostala przyznana.
		Failed     ///< Stan na przyszle nieudane questy.
	};

	/**
	 * @enum ObjectiveType
	 * @brief Typ celu sledzonego przez system questow.
	 */
	enum class ObjectiveType {
		Kill,            ///< Zabicie wskazanych przeciwnikow.
		CollectItem,     ///< Posiadanie albo zebranie przedmiotow.
		DeliverItem,     ///< Oddanie przedmiotu NPC.
		ReachCheckpoint, ///< Dotarcie do punktu kontrolnego.
		TalkToNPC        ///< Rozmowa z NPC.
	};

	/**
	 * @struct QuestObjective
	 * @brief Pojedynczy cel questa wraz z aktualnym postepem.
	 */
	struct QuestObjective {
		ObjectiveType type;
		std::string description;
		std::string target_name; ///< Nazwa wroga, NPC albo checkpointa.
		int item_id = 0;         ///< ID przedmiotu dla celow itemowych.
		int required_count = 1;
		int current_count = 0;

		/**
		 * @brief Sprawdza, czy cel zostal wykonany.
		 */
		[[nodiscard]] bool isCompleted() const { return current_count >= required_count; }

		/**
		 * @brief Zwieksza postep celu, nie przekraczajac wymaganej wartosci.
		 */
		void progress(int amount = 1) {
			current_count += amount;
			if (current_count > required_count)
				current_count = required_count;
		}

		/**
		 * @brief Zeruje postep celu.
		 */
		void reset() { current_count = 0; }
	};

	/**
	 * @struct QuestReward
	 * @brief Nagroda przyznawana po ukonczeniu questa.
	 */
	struct QuestReward {
		std::vector<int> item_ids;
		int gold = 0;
		int exp = 0;
	};

	/**
	 * @class Quest
	 * @brief Definicja questa, jego celow, wymagan i nagrod.
	 */
	class Quest {
	public:
		std::string id;
		std::string name;
		std::string description;
		std::string level_name; ///< Pusty tekst oznacza quest globalny.
		QuestState state = QuestState::Locked;
		bool auto_start = false;
		int required_level = 1;

		std::vector<std::string> prerequisites; ///< ID questow wymaganych przed startem.
		std::vector<QuestObjective> objectives;
		QuestReward reward;

		/**
		 * @brief Sprawdza, czy wszystkie cele zostaly wykonane.
		 */
		[[nodiscard]] bool areAllObjectivesComplete() const {
			for (const auto& objective : objectives) {
				if (!objective.isCompleted()) return false;
			}
			return true;
		}

		[[nodiscard]] bool isCompleted() const { return state == QuestState::Completed; }
		[[nodiscard]] bool isActive() const { return state == QuestState::Active; }
		[[nodiscard]] bool isAvailable() const { return state == QuestState::Available; }
		[[nodiscard]] bool isLocked() const { return state == QuestState::Locked; }
		[[nodiscard]] bool isFailed() const { return state == QuestState::Failed; }

		/**
		 * @brief Przenosi questa z dostepnego do aktywnego.
		 */
		void start() {
			if (state == QuestState::Available)
				state = QuestState::Active;
		}

		/**
		 * @brief Oznacza quest jako ukonczony.
		 */
		void complete() {
			state = QuestState::Completed;
		}

		/**
		 * @brief Oznacza quest jako nieudany.
		 */
		void fail() {
			state = QuestState::Failed;
		}

		/**
		 * @brief Przywraca stan poczatkowy i zeruje cele.
		 */
		void reset() {
			state = QuestState::Locked;
			for (auto& objective : objectives)
				objective.reset();
		}

		/**
		 * @brief Zwraca laczny postep w formacie "wykonane/wymagane".
		 */
		[[nodiscard]] std::string getProgressString() const {
			int done = 0;
			int total = 0;
			for (const auto& objective : objectives) {
				done += objective.current_count;
				total += objective.required_count;
			}
			return std::to_string(done) + "/" + std::to_string(total);
		}
	};

} // namespace Nawia::Game
