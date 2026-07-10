#include "StoryTriggerInternal.h"

#include "StoryTrigger.h"

#include <BossManager.h>
#include <Engine.h>
#include <EntityManager.h>
#include <Level.h>
#include <LevelManager.h>
#include <QuestManager.h>
#include <StoryConditions.h>

#include <string>

namespace Nawia::Entity::StoryTriggerSupport {

	namespace {

		std::shared_ptr<Entity> findEntityByName(Core::Engine* engine, const std::string& name) {
			if (!engine || name.empty())
				return nullptr;

			for (const auto& entity : engine->getEntityManager().getEntities()) {
				if (entity && entity->getName() == name)
					return entity;
			}
			return nullptr;
		}

		void hideEntity(Core::Engine& engine, const nlohmann::json& action) {
			const std::string entity_name = action.value("name", action.value("entity_name", ""));
			if (entity_name.empty())
				return;

			for (const auto& entity : engine.getEntityManager().getEntities()) {
				if (entity && entity->getName() == entity_name)
					entity->setDormant(true);
			}
		}

		void playEntityAnimation(Core::Engine& engine, const nlohmann::json& action) {
			const std::string entity_name = action.value("name", action.value("entity_name", ""));
			const std::string animation = action.value("animation", "");
			if (entity_name.empty() || animation.empty())
				return;

			for (const auto& entity : engine.getEntityManager().getEntities()) {
				if (!entity || entity->getName() != entity_name)
					continue;

				if (action.value("face_player", false)) {
					if (const auto& player = engine.getPlayer())
						entity->rotateTowardsCenter(player->getCenter().x, player->getCenter().y);
				}

				entity->setAnimationSpeed(action.value("animation_speed", 1.0f));
				if (action.value("freeze_last_frame", false)) {
					entity->playAnimationFreezeOnLastFrame(
						animation,
						action.value("lock_movement", false),
						action.value("start_frame", 0),
						true
					);
				} else {
					entity->playAnimation(
						animation,
						action.value("loop", false),
						action.value("lock_movement", false),
						action.value("start_frame", 0),
						true
					);
				}
				break;
			}
		}

	}

	void executeActionData(Core::Engine* engine, StoryTrigger* trigger, const nlohmann::json& action) {
		if (!engine || !action.is_object())
			return;

		if (!Game::areStoryConditionsMet(action.value("conditions", nlohmann::json::object()), engine))
			return;

		const std::string type = action.value("type", "");
		if (type == "notify_checkpoint") {
			const std::string name = action.value("name", action.value("checkpoint", ""));
			if (!name.empty()) {
				engine->getQuestManager().notifyCheckpointReached(name);
				engine->getQuestManager().update(engine);
			}
		} else if (type == "start_quest") {
			const std::string quest_id = action.value("quest_id", "");
			if (!quest_id.empty())
				engine->getQuestManager().startQuest(quest_id);
		} else if (type == "complete_quest") {
			const std::string quest_id = action.value("quest_id", "");
			if (!quest_id.empty()) {
				engine->getQuestManager().completeQuest(quest_id, engine);
				engine->getQuestManager().update(engine);
			}
		} else if (type == "fail_quest") {
			const std::string quest_id = action.value("quest_id", "");
			if (!quest_id.empty())
				engine->getQuestManager().failQuest(quest_id, engine);
		} else if (type == "teleport") {
			const std::string target_location = action.value("target_location", "");
			if (!target_location.empty()) {
				if (auto* current_level = engine->getLevelManager().getCurrentLevel())
					current_level->changeLocation(engine, target_location);
			}
		} else if (type == "start_boss") {
			const std::string boss_id = action.value("boss_id", "");
			if (!boss_id.empty() && trigger)
				engine->getBossManager().startBossFight(boss_id, engine, trigger->getCenter(), trigger->getAltitude());
		} else if (type == "hide_entity") {
			hideEntity(*engine, action);
		} else if (type == "play_entity_animation") {
			playEntityAnimation(*engine, action);
		}
	}

	std::vector<nlohmann::json> collectActions(const nlohmann::json& data) {
		std::vector<nlohmann::json> actions;
		if (data.contains("actions") && data["actions"].is_array()) {
			for (const auto& action : data["actions"]) {
				if (action.is_object())
					actions.push_back(action);
			}
		}

		if (data.contains("on_complete") && data["on_complete"].is_array()) {
			for (const auto& action : data["on_complete"]) {
				if (action.is_object())
					actions.push_back(action);
			}
		}

		const std::string checkpoint = data.value("checkpoint_on_complete", data.value("notify_checkpoint", ""));
		if (!checkpoint.empty())
			actions.push_back({{"type", "notify_checkpoint"}, {"name", checkpoint}});

		const std::string start_quest = data.value("start_quest", "");
		if (!start_quest.empty())
			actions.push_back({{"type", "start_quest"}, {"quest_id", start_quest}});

		const std::string complete_quest = data.value("complete_quest", "");
		if (!complete_quest.empty())
			actions.push_back({{"type", "complete_quest"}, {"quest_id", complete_quest}});

		const std::string fail_quest = data.value("fail_quest", "");
		if (!fail_quest.empty())
			actions.push_back({{"type", "fail_quest"}, {"quest_id", fail_quest}});

		const std::string target_location = data.value("target_location", "");
		if (!target_location.empty())
			actions.push_back({{"type", "teleport"}, {"target_location", target_location}});

		const std::string boss_id = data.contains("start_boss")
			? data.value("start_boss", "")
			: (data.value("action", "") == "start_boss" ? data.value("boss_id", "") : "");
		if (!boss_id.empty())
			actions.push_back({{"type", "start_boss"}, {"boss_id", boss_id}});

		return actions;
	}

	std::vector<nlohmann::json> collectEnterActions(const nlohmann::json& data) {
		std::vector<nlohmann::json> actions;
		if (data.contains("on_enter") && data["on_enter"].is_array()) {
			for (const auto& action : data["on_enter"]) {
				if (action.is_object())
					actions.push_back(action);
			}
		}
		return actions;
	}

	std::shared_ptr<Entity> resolveDialogueSpeaker(Core::Engine* engine, const nlohmann::json& data) {
		std::string speaker_name = data.value("dialogue_speaker", data.value("dialogue_target", ""));
		if (speaker_name.empty() && data.contains("on_enter") && data["on_enter"].is_array()) {
			for (const auto& action : data["on_enter"]) {
				if (!action.is_object())
					continue;

				if (action.value("type", "") == "play_entity_animation") {
					speaker_name = action.value("name", action.value("entity_name", ""));
					if (!speaker_name.empty())
						break;
				}
			}
		}

		return findEntityByName(engine, speaker_name);
	}

} // namespace Nawia::Entity::StoryTriggerSupport
