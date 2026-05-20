#pragma once

#include <Quest.h>
#include <json.hpp>

#include <map>
#include <string>
#include <vector>

namespace Nawia::Core { class Engine; }

namespace Nawia::Game {

	/**
	 * @class QuestManager
	 * @brief Wczytuje questy, sledzi ich postep i obsluguje odblokowania lancuchow.
	 */
	class QuestManager {
	public:
		/**
		 * @brief Wczytuje definicje questow z pliku JSON.
		 */
		void loadFromJson(const std::string& path);

		/**
		 * @brief Przywraca wszystkie questy do stanu poczatkowego.
		 */
		void resetAll();

		/**
		 * @brief Uruchamia dostepny quest o podanym ID.
		 */
		bool startQuest(const std::string& id);

		/**
		 * @brief Wymusza ukonczenie questa i przyznaje nagrody.
		 */
		void completeQuest(const std::string& id, Core::Engine* engine);

		/**
		 * @brief Zwraca quest po ID albo nullptr.
		 */
		[[nodiscard]] Quest* getQuest(const std::string& id);
		[[nodiscard]] const Quest* getQuest(const std::string& id) const;

		/**
		 * @brief Ustawia nazwe aktualnego poziomu dla filtrowania questow.
		 */
		void setCurrentLevel(const std::string& level_name);

		/**
		 * @brief Zwraca aktywne questy z aktualnego poziomu.
		 */
		[[nodiscard]] std::vector<Quest*> getActiveQuests();

		/**
		 * @brief Zwraca dostepne questy z aktualnego poziomu.
		 */
		[[nodiscard]] std::vector<Quest*> getAvailableQuests();

		/**
		 * @brief Zwraca ukonczone questy z aktualnego poziomu.
		 */
		[[nodiscard]] std::vector<Quest*> getCompletedQuests();

		/**
		 * @brief Zwraca questy przypisane do poziomu oraz questy globalne.
		 */
		[[nodiscard]] std::vector<Quest*> getQuestsForLevel(const std::string& level_name);

		/**
		 * @brief Informuje system o zabiciu przeciwnika.
		 */
		void notifyKill(const std::string& enemy_name);

		/**
		 * @brief Informuje system o zebraniu przedmiotu.
		 */
		void notifyItemCollected(int item_id);

		/**
		 * @brief Informuje system o oddaniu przedmiotu NPC.
		 */
		void notifyItemDelivered(int item_id, const std::string& npc_name);

		/**
		 * @brief Informuje system o dotarciu do checkpointa.
		 */
		void notifyCheckpointReached(const std::string& checkpoint_name);

		/**
		 * @brief Informuje system o rozmowie z NPC.
		 */
		void notifyNPCTalked(const std::string& npc_name);

		/**
		 * @brief Odblokowuje dostepne questy i zamyka wykonane aktywne questy.
		 */
		void update(Core::Engine* engine);

		[[nodiscard]] nlohmann::json serializeState() const;
		void applyState(const nlohmann::json& state);

	private:
		[[nodiscard]] bool arePrerequisitesMet(const Quest& quest, Core::Engine* engine) const;
		[[nodiscard]] bool isQuestForCurrentLevel(const Quest& quest) const;
		[[nodiscard]] std::vector<Quest*> getQuestsByState(QuestState state);

		std::map<std::string, Quest> _quests;
		std::string _current_level;
	};

} // namespace Nawia::Game
