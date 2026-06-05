#include "StoryConditions.h"

#include <BossManager.h>
#include <Engine.h>
#include <QuestManager.h>

#include <initializer_list>
#include <string>
#include <vector>

namespace Nawia::Game {

	namespace {

		std::vector<std::string> readStringList(
			const nlohmann::json& data,
			const std::initializer_list<const char*> keys
		) {
			std::vector<std::string> result;

			for (const char* key : keys) {
				const auto it = data.find(key);
				if (it == data.end())
					continue;

				if (it->is_string()) {
					result.push_back(it->get<std::string>());
				} else if (it->is_array()) {
					for (const auto& entry : *it) {
						if (entry.is_string())
							result.push_back(entry.get<std::string>());
					}
				}
			}

			return result;
		}

		template <typename Predicate>
		bool allQuestIdsMatch(Core::Engine* engine, const std::vector<std::string>& ids, Predicate predicate) {
			if (ids.empty())
				return true;

			if (!engine)
				return false;

			for (const auto& id : ids) {
				const Quest* quest = engine->getQuestManager().getQuest(id);
				if (!quest || !predicate(*quest))
					return false;
			}

			return true;
		}

		template <typename Predicate>
		bool noQuestIdsMatch(Core::Engine* engine, const std::vector<std::string>& ids, Predicate predicate) {
			if (ids.empty())
				return true;

			if (!engine)
				return false;

			for (const auto& id : ids) {
				const Quest* quest = engine->getQuestManager().getQuest(id);
				if (quest && predicate(*quest))
					return false;
			}

			return true;
		}

		bool allBossesDefeated(Core::Engine* engine, const std::vector<std::string>& ids) {
			if (ids.empty())
				return true;

			if (!engine)
				return false;

			for (const auto& id : ids) {
				if (!engine->getBossManager().isBossDefeated(id))
					return false;
			}

			return true;
		}

		bool noBossesDefeated(Core::Engine* engine, const std::vector<std::string>& ids) {
			if (ids.empty())
				return true;

			if (!engine)
				return false;

			for (const auto& id : ids) {
				if (engine->getBossManager().isBossDefeated(id))
					return false;
			}

			return true;
		}

	}

	bool areStoryConditionsMet(const nlohmann::json& data, Core::Engine* engine) {
		if (!data.is_object())
			return true;

		const auto required_completed = readStringList(data, {
			"required_quest_completed",
			"required_quests_completed"
		});
		const auto required_active = readStringList(data, {
			"required_active_quest",
			"required_quest_active",
			"required_quests_active"
		});
		const auto required_failed = readStringList(data, {
			"required_quest_failed",
			"required_quests_failed"
		});
		const auto blocked_completed = readStringList(data, {
			"blocking_completed_quest",
			"blocked_quest_completed",
			"blocked_quests_completed"
		});
		const auto blocked_active = readStringList(data, {
			"blocking_active_quest",
			"blocked_quest_active",
			"blocked_quests_active"
		});
		const auto blocked_failed = readStringList(data, {
			"blocking_failed_quest",
			"blocked_quest_failed",
			"blocked_quests_failed"
		});

		if (!allQuestIdsMatch(engine, required_completed, [](const Quest& quest) { return quest.isCompleted(); }))
			return false;
		if (!allQuestIdsMatch(engine, required_active, [](const Quest& quest) { return quest.isActive(); }))
			return false;
		if (!allQuestIdsMatch(engine, required_failed, [](const Quest& quest) { return quest.isFailed(); }))
			return false;

		if (!noQuestIdsMatch(engine, blocked_completed, [](const Quest& quest) { return quest.isCompleted(); }))
			return false;
		if (!noQuestIdsMatch(engine, blocked_active, [](const Quest& quest) { return quest.isActive(); }))
			return false;
		if (!noQuestIdsMatch(engine, blocked_failed, [](const Quest& quest) { return quest.isFailed(); }))
			return false;

		const auto required_bosses = readStringList(data, {
			"required_boss_defeated",
			"required_bosses_defeated"
		});
		const auto blocked_bosses = readStringList(data, {
			"blocked_boss_defeated",
			"blocked_bosses_defeated"
		});

		return allBossesDefeated(engine, required_bosses) &&
			noBossesDefeated(engine, blocked_bosses);
	}

	bool areEntityConditionsMet(const nlohmann::json& entity_data, Core::Engine* engine) {
		if (!entity_data.is_object())
			return true;

		if (!areStoryConditionsMet(entity_data, engine))
			return false;

		const auto conditions_it = entity_data.find("conditions");
		if (conditions_it != entity_data.end() && !areStoryConditionsMet(*conditions_it, engine))
			return false;

		const auto spawn_if_it = entity_data.find("spawn_if");
		if (spawn_if_it != entity_data.end() && !areStoryConditionsMet(*spawn_if_it, engine))
			return false;

		const auto hide_if_it = entity_data.find("hide_if");
		if (hide_if_it != entity_data.end() && areStoryConditionsMet(*hide_if_it, engine))
			return false;

		return true;
	}

} // namespace Nawia::Game
