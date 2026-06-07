#include "QuestManager.h"

#include <Engine.h>
#include <Item.h>
#include <Logger.h>
#include <Player.h>

#include <json.hpp>

#include <algorithm>
#include <fstream>
#include <utility>

using json = nlohmann::json;

namespace Nawia::Game {

	namespace {

		ObjectiveType parseObjectiveType(const std::string& type_text) {
			if (type_text == "kill") return ObjectiveType::Kill;
			if (type_text == "collect_item") return ObjectiveType::CollectItem;
			if (type_text == "deliver_item") return ObjectiveType::DeliverItem;
			if (type_text == "reach_checkpoint") return ObjectiveType::ReachCheckpoint;
			if (type_text == "talk_to_npc") return ObjectiveType::TalkToNPC;

			Core::Logger::errorLog("QuestManager: nieznany typ celu: " + type_text);
			return ObjectiveType::Kill;
		}

		std::string questStateToString(const QuestState state) {
			switch (state) {
				case QuestState::Locked: return "locked";
				case QuestState::Available: return "available";
				case QuestState::Active: return "active";
				case QuestState::Completed: return "completed";
				case QuestState::Failed: return "failed";
			}

			return "locked";
		}

		QuestState questStateFromString(const std::string& state) {
			if (state == "available") return QuestState::Available;
			if (state == "active") return QuestState::Active;
			if (state == "completed") return QuestState::Completed;
			if (state == "failed") return QuestState::Failed;
			return QuestState::Locked;
		}

		QuestState getInitialState(const Quest& quest) {
			return (!quest.level_name.empty() || !quest.prerequisites.empty())
				? QuestState::Locked
				: QuestState::Available;
		}

		void loadPrerequisites(Quest& quest, const json& quest_json) {
			if (!quest_json.contains("prerequisites"))
				return;

			for (const auto& prerequisite : quest_json["prerequisites"]) {
				quest.prerequisites.push_back(prerequisite.get<std::string>());
			}
		}

		void loadObjectives(Quest& quest, const json& quest_json) {
			if (!quest_json.contains("objectives"))
				return;

			for (const auto& objective_json : quest_json["objectives"]) {
				QuestObjective objective;
				objective.type = parseObjectiveType(objective_json.value("type", "kill"));
				objective.description = objective_json.value("description", "");
				objective.target_name = objective_json.value("target_name", objective_json.value("target_npc", ""));
				objective.item_id = objective_json.value("item_id", 0);
				objective.required_count = objective_json.value("count", 1);
				objective.current_count = 0;
				quest.objectives.push_back(objective);
			}
		}

		void loadRewards(Quest& quest, const json& quest_json) {
			if (!quest_json.contains("rewards"))
				return;

			const auto& rewards_json = quest_json["rewards"];
			if (rewards_json.contains("items")) {
				for (const auto& item_id : rewards_json["items"]) {
					quest.reward.item_ids.push_back(item_id.get<int>());
				}
			}
			quest.reward.gold = rewards_json.value("gold", 0);
			quest.reward.exp = rewards_json.value("exp", 0);
		}

		Quest parseQuest(const json& quest_json) {
			Quest quest;
			quest.id = quest_json.value("id", "");
			quest.name = quest_json.value("name", "");
			quest.description = quest_json.value("description", "");
			quest.level_name = quest_json.value("level_name", "");
			quest.auto_start = quest_json.value("auto_start", false);
			quest.required_level = quest_json.value("required_level", 1);

			loadPrerequisites(quest, quest_json);
			loadObjectives(quest, quest_json);
			loadRewards(quest, quest_json);
			quest.state = getInitialState(quest);

			return quest;
		}

		template <typename Predicate>
		void progressMatchingObjectives(
			std::map<std::string, Quest>& quests,
			ObjectiveType objective_type,
			Predicate predicate
		) {
			for (auto& [id, quest] : quests) {
				if (!quest.isActive()) continue;

				for (auto& objective : quest.objectives) {
					if (objective.type == objective_type && !objective.isCompleted() && predicate(objective)) {
						objective.progress();
					}
				}
			}
		}

	}

	void QuestManager::loadFromJson(const std::string& path) {
		std::ifstream file(path);
		if (!file.is_open()) {
			Core::Logger::errorLog("QuestManager: nie mozna otworzyc pliku: " + path);
			return;
		}

		json data;
		try {
			file >> data;
		} catch (const json::parse_error&) {
			Core::Logger::errorLog("QuestManager: blad parsowania JSON w pliku: " + path);
			return;
		}

		if (!data.contains("quests") || !data["quests"].is_array()) {
			Core::Logger::errorLog("QuestManager: brak tablicy 'quests' w pliku: " + path);
			return;
		}

		_quests.clear();

		for (const auto& quest_json : data["quests"]) {
			Quest quest = parseQuest(quest_json);
			if (quest.id.empty())
				continue;

			const std::string quest_id = quest.id;
			_quests[quest_id] = std::move(quest);
			Core::Logger::debugLog("QuestManager: zaladowano quest '" + quest_id + "'");
		}

		Core::Logger::debugLog("QuestManager: zaladowano questy, liczba: " + std::to_string(_quests.size()));
	}

	void QuestManager::resetAll() {
		for (auto& [id, quest] : _quests) {
			quest.reset();
			quest.state = getInitialState(quest);
		}
	}

	void QuestManager::setCurrentLevel(const std::string& level_name) {
		_current_level = level_name;
		Core::Logger::debugLog("QuestManager: aktualny poziom to '" + level_name + "'");

		for (auto& [id, quest] : _quests) {
			if (!isQuestForCurrentLevel(quest)) continue;

			if (quest.isLocked() && quest.prerequisites.empty()) {
				quest.state = QuestState::Available;
				Core::Logger::debugLog("QuestManager: quest '" + id + "' odblokowany dla poziomu '" + level_name + "'");
			}

			if (quest.isAvailable() && quest.auto_start) {
				quest.start();
				Core::Logger::debugLog("QuestManager: automatycznie wystartowano quest '" + id + "'");
			}
		}
	}

	bool QuestManager::isQuestForCurrentLevel(const Quest& quest) const {
		return quest.level_name.empty() || quest.level_name == _current_level;
	}

	bool QuestManager::startQuest(const std::string& id) {
		Quest* quest = getQuest(id);
		if (!quest || quest->state != QuestState::Available)
			return false;

		quest->start();
		Core::Logger::debugLog("QuestManager: wystartowano quest '" + id + "'");
		return true;
	}

	void QuestManager::completeQuest(const std::string& id, Core::Engine* engine) {
		Quest* quest = getQuest(id);
		if (!quest || quest->state != QuestState::Active)
			return;

		quest->complete();
		Core::Logger::debugLog("QuestManager: ukonczono quest '" + id + "'");

		if (!engine)
			return;

		const auto player = engine->getPlayer();
		if (!player)
			return;

		if (quest->reward.gold > 0)
			player->addGold(quest->reward.gold);

		if (quest->reward.exp > 0)
			player->addExp(quest->reward.exp);

		for (const int item_id : quest->reward.item_ids) {
			if (auto item = engine->getItemDatabase().createItem(item_id)) {
				if (item->isFood())
					player->addFood(1);
				else if (item->getSlot() == Item::EquipmentSlot::Weapon)
					player->equipItem(item);
				else
					player->getBackpack().addItem(item);
			}
		}

		std::string notification = "Quest ukonczony: " + quest->name;
		if (quest->reward.exp > 0)
			notification += " (+" + std::to_string(quest->reward.exp) + " XP)";

		engine->getUIHandler().showNotification(notification, 4.0f);
	}

	bool QuestManager::failQuest(const std::string& id, Core::Engine* engine) {
		Quest* quest = getQuest(id);
		if (!quest || quest->isCompleted() || quest->isFailed())
			return false;

		quest->fail();
		Core::Logger::debugLog("QuestManager: quest '" + id + "' nieudany");

		if (engine)
			engine->getUIHandler().showNotification("Quest nieudany: " + quest->name, 4.0f);

		return true;
	}

	Quest* QuestManager::getQuest(const std::string& id) {
		const auto quest_it = _quests.find(id);
		return quest_it != _quests.end() ? &quest_it->second : nullptr;
	}

	const Quest* QuestManager::getQuest(const std::string& id) const {
		const auto quest_it = _quests.find(id);
		return quest_it != _quests.end() ? &quest_it->second : nullptr;
	}

	std::vector<Quest*> QuestManager::getQuestsByState(const QuestState state) {
		std::vector<Quest*> result;
		for (auto& [id, quest] : _quests) {
			if (quest.state == state && isQuestForCurrentLevel(quest))
				result.push_back(&quest);
		}
		return result;
	}

	std::vector<Quest*> QuestManager::getActiveQuests() {
		return getQuestsByState(QuestState::Active);
	}

	std::vector<Quest*> QuestManager::getAvailableQuests() {
		return getQuestsByState(QuestState::Available);
	}

	std::vector<Quest*> QuestManager::getCompletedQuests() {
		return getQuestsByState(QuestState::Completed);
	}

	std::vector<Quest*> QuestManager::getFailedQuests() {
		return getQuestsByState(QuestState::Failed);
	}

	std::vector<Quest*> QuestManager::getQuestsForLevel(const std::string& level_name) {
		std::vector<Quest*> result;
		for (auto& [id, quest] : _quests) {
			if (quest.level_name.empty() || quest.level_name == level_name)
				result.push_back(&quest);
		}
		return result;
	}

	void QuestManager::notifyKill(const std::string& enemy_name) {
		progressMatchingObjectives(
			_quests,
			ObjectiveType::Kill,
			[&enemy_name](const QuestObjective& objective) {
				return objective.target_name == enemy_name;
			}
		);
	}

	void QuestManager::notifyItemCollected(const int item_id) {
		progressMatchingObjectives(
			_quests,
			ObjectiveType::CollectItem,
			[item_id](const QuestObjective& objective) {
				return objective.item_id == item_id;
			}
		);
	}

	void QuestManager::notifyItemDelivered(const int item_id, const std::string& npc_name) {
		progressMatchingObjectives(
			_quests,
			ObjectiveType::DeliverItem,
			[item_id, &npc_name](const QuestObjective& objective) {
				return objective.item_id == item_id && objective.target_name == npc_name;
			}
		);
	}

	void QuestManager::notifyCheckpointReached(const std::string& checkpoint_name) {
		progressMatchingObjectives(
			_quests,
			ObjectiveType::ReachCheckpoint,
			[&checkpoint_name](const QuestObjective& objective) {
				return objective.target_name == checkpoint_name;
			}
		);
	}

	void QuestManager::notifyNPCTalked(const std::string& npc_name) {
		progressMatchingObjectives(
			_quests,
			ObjectiveType::TalkToNPC,
			[&npc_name](const QuestObjective& objective) {
				return objective.target_name == npc_name;
			}
		);
	}

	bool QuestManager::arePrerequisitesMet(const Quest& quest, Core::Engine* engine) const {
		for (const auto& prerequisite_id : quest.prerequisites) {
			const Quest* prerequisite = getQuest(prerequisite_id);
			if (!prerequisite || !prerequisite->isCompleted())
				return false;
		}

		if (engine && engine->getPlayer() && engine->getPlayer()->getLevel() < quest.required_level)
			return false;

		return true;
	}

	void QuestManager::update(Core::Engine* engine) {
		for (auto& [id, quest] : _quests) {
			if (!isQuestForCurrentLevel(quest)) continue;

			if (quest.isLocked() && arePrerequisitesMet(quest, engine)) {
				quest.state = QuestState::Available;
				Core::Logger::debugLog("QuestManager: quest '" + id + "' jest teraz dostepny.");

				if (engine && quest.auto_start)
					engine->getUIHandler().showNotification("Nowy quest dostepny: " + quest.name, 3.0f);

				if (quest.auto_start) {
					quest.start();
					Core::Logger::debugLog("QuestManager: automatycznie wystartowano quest '" + id + "'");
				}
			}

			if (quest.isActive() && quest.areAllObjectivesComplete())
				completeQuest(id, engine);
		}
	}

	json QuestManager::serializeState() const {
		json quests = json::array();

		for (const auto& [id, quest] : _quests) {
			json quest_state;
			quest_state["id"] = id;
			quest_state["level_name"] = quest.level_name;
			quest_state["state"] = questStateToString(quest.state);
			quest_state["objectives"] = json::array();

			for (size_t i = 0; i < quest.objectives.size(); ++i) {
				const auto& objective = quest.objectives[i];
				quest_state["objectives"].push_back({
					{"index", i},
					{"current_count", objective.current_count}
				});
			}

			quests.push_back(std::move(quest_state));
		}

		return {
			{"current_level", _current_level},
			{"quests", std::move(quests)}
		};
	}

	void QuestManager::applyState(const json& state) {
		if (state.contains("current_level") && state["current_level"].is_string())
			_current_level = state["current_level"].get<std::string>();

		if (!state.contains("quests") || !state["quests"].is_array())
			return;

		for (const auto& quest_state : state["quests"]) {
			const std::string id = quest_state.value("id", "");
			auto quest_it = _quests.find(id);
			if (quest_it == _quests.end())
				continue;

			Quest& quest = quest_it->second;
			quest.state = questStateFromString(quest_state.value("state", "locked"));

			if (!quest_state.contains("objectives") || !quest_state["objectives"].is_array())
				continue;

			for (const auto& objective_state : quest_state["objectives"]) {
				const int index = objective_state.value("index", -1);
				if (index < 0 || static_cast<size_t>(index) >= quest.objectives.size())
					continue;

				auto& objective = quest.objectives[static_cast<size_t>(index)];
				objective.current_count = std::clamp(
					objective_state.value("current_count", 0),
					0,
					objective.required_count
				);
			}
		}
	}

} // namespace Nawia::Game
