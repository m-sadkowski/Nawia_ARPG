#include "QuestManager.h"

#include <Engine.h>
#include <Logger.h>

#include <json.hpp>
#include <fstream>

namespace Nawia::Game {

	// ─── JSON Loading ─────────────────────────────────────

	static ObjectiveType parseObjectiveType(const std::string& type_str) {
		if (type_str == "kill")				return ObjectiveType::Kill;
		if (type_str == "collect_item")		return ObjectiveType::CollectItem;
		if (type_str == "deliver_item")		return ObjectiveType::DeliverItem;
		if (type_str == "reach_checkpoint")	return ObjectiveType::ReachCheckpoint;
		if (type_str == "talk_to_npc")		return ObjectiveType::TalkToNPC;

		Core::Logger::errorLog("QuestManager: Unknown objective type: " + type_str);
		return ObjectiveType::Kill;
	}

	void QuestManager::loadFromJson(const std::string& path) {
		std::ifstream file(path);
		if (!file.is_open()) {
			Core::Logger::errorLog("QuestManager: Could not open " + path);
			return;
		}

		nlohmann::json data;
		try {
			file >> data;
		}
		catch (const nlohmann::json::parse_error& e) {
			(void)e;
			Core::Logger::errorLog("QuestManager: JSON parse error in " + path);
			return;
		}

		if (!data.contains("quests")) {
			Core::Logger::errorLog("QuestManager: No 'quests' array in " + path);
			return;
		}

		_quests.clear();

		for (const auto& qj : data["quests"]) {
			Quest quest;
			quest.id = qj.value("id", "");
			quest.name = qj.value("name", "");
			quest.description = qj.value("description", "");
			quest.auto_start = qj.value("auto_start", false);
			quest.required_level = qj.value("required_level", 1);

			// Prerequisites
			if (qj.contains("prerequisites")) {
				for (const auto& pre : qj["prerequisites"]) {
					quest.prerequisites.push_back(pre.get<std::string>());
				}
			}

			// Objectives
			if (qj.contains("objectives")) {
				for (const auto& oj : qj["objectives"]) {
					QuestObjective obj;
					obj.type = parseObjectiveType(oj.value("type", "kill"));
					obj.description = oj.value("description", "");
					obj.target_name = oj.value("target_name", oj.value("target_npc", ""));
					obj.item_id = oj.value("item_id", 0);
					obj.required_count = oj.value("count", 1);
					obj.current_count = 0;
					quest.objectives.push_back(obj);
				}
			}

			// Rewards
			if (qj.contains("rewards")) {
				const auto& rj = qj["rewards"];
				if (rj.contains("items")) {
					for (const auto& item_id : rj["items"]) {
						quest.reward.item_ids.push_back(item_id.get<int>());
					}
				}
				quest.reward.gold = rj.value("gold", 0);
				quest.reward.exp = rj.value("exp", 0);
			}

			// Initial state
			quest.state = quest.prerequisites.empty() ? QuestState::Available : QuestState::Locked;

			if (!quest.id.empty()) {
				_quests[quest.id] = quest;
				Core::Logger::debugLog("QuestManager: Loaded quest '" + quest.id + "'");
			}
		}

		Core::Logger::debugLog("QuestManager: Loaded " + std::to_string(_quests.size()) + " quests.");
	}

	// ─── Quest State Management ───────────────────────────

	void QuestManager::resetAll() {
		for (auto& [id, quest] : _quests) {
			quest.reset();
			quest.state = quest.prerequisites.empty() ? QuestState::Available : QuestState::Locked;
		}
	}

	bool QuestManager::startQuest(const std::string& id) {
		Quest* quest = getQuest(id);
		if (!quest) return false;

		if (quest->state != QuestState::Available) return false;

		quest->start();
		Core::Logger::debugLog("QuestManager: Started quest '" + id + "'");
		return true;
	}

	void QuestManager::completeQuest(const std::string& id, Core::Engine* engine) {
		Quest* quest = getQuest(id);
		if (!quest || quest->state != QuestState::Active) return;

		quest->complete();
		Core::Logger::debugLog("QuestManager: Completed quest '" + id + "'");

		// Give rewards
		if (engine) {
			auto player = engine->getPlayer();
			if (player) {
				// Gold
				if (quest->reward.gold > 0) {
					player->addGold(quest->reward.gold);
					std::cout << player->getGold();
					std::cout << std::endl;
				}

				// Exp
				if (quest->reward.exp > 0) {
					player->addExp(quest->reward.exp);
					std::cout << player->getExp();
					std::cout << std::endl;
				}

				// Items
				for (const int item_id : quest->reward.item_ids) {
					if (auto item = engine->getItemDatabase().createItem(item_id)) {
						player->getBackpack().addItem(item);
					}
				}

				// Notification
				std::string notif = "Quest ukonczony: " + quest->name;
				if (quest->reward.exp > 0) {
					notif += " (+" + std::to_string(quest->reward.exp) + " XP)";
				}
				engine->getUIHandler().showNotification(notif, 4.0f);
			}
		}
	}

	Quest* QuestManager::getQuest(const std::string& id) {
		auto it = _quests.find(id);
		return it != _quests.end() ? &it->second : nullptr;
	}

	const Quest* QuestManager::getQuest(const std::string& id) const {
		auto it = _quests.find(id);
		return it != _quests.end() ? &it->second : nullptr;
	}

	std::vector<Quest*> QuestManager::getActiveQuests() {
		std::vector<Quest*> result;
		for (auto& [id, quest] : _quests) {
			std::cout << "sprawdzamy czy quset jest aktywny?";
			if (quest.isActive()) result.push_back(&quest);
		}
		return result;
	}

	std::vector<Quest*> QuestManager::getAvailableQuests() {
		std::vector<Quest*> result;
		for (auto& [id, quest] : _quests) {
			if (quest.isAvailable()) result.push_back(&quest);
		}
		return result;
	}

	std::vector<Quest*> QuestManager::getCompletedQuests() {
		std::vector<Quest*> result;
		for (auto& [id, quest] : _quests) {
			if (quest.isCompleted()) result.push_back(&quest);
		}
		return result;
	}

	// ─── Notification System ──────────────────────────────

	void QuestManager::notifyKill(const std::string& enemy_name) {
		for (auto& [id, quest] : _quests) {
			if (!quest.isActive()) continue;
			for (auto& obj : quest.objectives) {
				if (obj.type == ObjectiveType::Kill && obj.target_name == enemy_name && !obj.isCompleted()) {
					obj.progress();
					Core::Logger::debugLog("QuestManager: Kill progress '" + enemy_name + "' for quest '" + id + "' (" + std::to_string(obj.current_count) + "/" + std::to_string(obj.required_count) + ")");
				}
			}
		}
	}

	void QuestManager::notifyItemCollected(const int item_id) {
		for (auto& [id, quest] : _quests) {
			if (!quest.isActive()) continue;
			for (auto& obj : quest.objectives) {
				if (obj.type == ObjectiveType::CollectItem && obj.item_id == item_id && !obj.isCompleted()) {
					obj.progress();
				}
			}
		}
	}

	void QuestManager::notifyItemDelivered(const int item_id, const std::string& npc_name) {
		for (auto& [id, quest] : _quests) {
			if (!quest.isActive()) continue;
			for (auto& obj : quest.objectives) {
				if (obj.type == ObjectiveType::DeliverItem && obj.item_id == item_id && obj.target_name == npc_name && !obj.isCompleted()) {
					obj.progress();
				}
			}
		}
	}

	void QuestManager::notifyCheckpointReached(const std::string& checkpoint_name) {
		for (auto& [id, quest] : _quests) {
			if (!quest.isActive()) continue;
			for (auto& obj : quest.objectives) {
				if (obj.type == ObjectiveType::ReachCheckpoint && obj.target_name == checkpoint_name && !obj.isCompleted()) {
					obj.progress();
				}
			}
		}
	}

	void QuestManager::notifyNPCTalked(const std::string& npc_name) {
		for (auto& [id, quest] : _quests) {
			if (!quest.isActive()) continue;
			for (auto& obj : quest.objectives) {
				if (obj.type == ObjectiveType::TalkToNPC && obj.target_name == npc_name && !obj.isCompleted()) {
					obj.progress();
				}
			}
		}
	}

	// ─── Update Loop ──────────────────────────────────────

	bool QuestManager::arePrerequisitesMet(const Quest& quest, Core::Engine* engine) const {
		for (const auto& pre_id : quest.prerequisites) {
			const Quest* pre = getQuest(pre_id);
			if (!pre || !pre->isCompleted()) return false;
		}
		
		if (engine && engine->getPlayer()) {
			if (engine->getPlayer()->getLevel() < quest.required_level) {
				return false;
			}
		}

		return true;
	}

	void QuestManager::update(Core::Engine* engine) {
		for (auto& [id, quest] : _quests) {
			// Unlock locked quests whose prerequisites are now met
			if (quest.isLocked() && arePrerequisitesMet(quest, engine)) {
				quest.state = QuestState::Available;
				Core::Logger::debugLog("QuestManager: Quest '" + id + "' is now available.");

				if (engine) {
					engine->getUIHandler().showNotification("Nowy quest dostepny: " + quest.name, 3.0f);
				}

				// Auto-start if flagged
				if (quest.auto_start) {
					quest.start();
					Core::Logger::debugLog("QuestManager: Auto-started quest '" + id + "'");
				}
			}

			// Auto-complete active quests with all objectives done
			if (quest.isActive() && quest.areAllObjectivesComplete()) {
				completeQuest(id, engine);
			}
		}
	}

} // namespace Nawia::Game
