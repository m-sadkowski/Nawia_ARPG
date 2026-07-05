#include "StoryTrigger.h"

#include <BossManager.h>
#include <Collider.h>
#include <Dialogue.h>
#include <Engine.h>
#include <EntityManager.h>
#include <Level.h>
#include <LevelManager.h>
#include <Logger.h>
#include <QuestManager.h>
#include <StoryConditions.h>
#include <UIHandler.h>

#include <fstream>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace Nawia::Entity {

	namespace {

		bool isPlayerDialogueSpeaker(const std::string& speaker) {
			return speaker == "Logos" || speaker == "Jarko" || speaker == "Player" || speaker == "Gracz";
		}

		bool isPlaceholderOption(const std::string& text) {
			return text.empty() || text == "..." || text == "Dalej";
		}

		std::string resolveFinalOption(
			const std::string& configured_text,
			const std::string& current_speaker,
			const std::string& current_text
		) {
			if (!isPlaceholderOption(configured_text))
				return configured_text;

			return isPlayerDialogueSpeaker(current_speaker) ? current_text : "Rozumiem.";
		}

		nlohmann::json loadJsonDocument(const std::string& path) {
			std::ifstream file(path);
			if (!file.is_open()) {
				Core::Logger::errorLog("StoryTrigger: nie mozna otworzyc JSON: " + path);
				return {};
			}

			nlohmann::json data;
			try {
				file >> data;
			} catch (const nlohmann::json::parse_error&) {
				Core::Logger::errorLog("StoryTrigger: blad parsowania JSON: " + path);
				return {};
			}
			return data;
		}

		const nlohmann::json& getNpcDialogueConfig() {
			static const nlohmann::json config = loadJsonDocument("assets/data/npc_dialogues.json");
			return config;
		}

		nlohmann::json resolveDialogueJson(const nlohmann::json& data) {
			if (data.contains("dialogue") && data["dialogue"].is_object())
				return data["dialogue"];

			const std::string dialogue_key = data.value("dialogue_key", "");
			if (dialogue_key.empty())
				return {};

			const auto& config = getNpcDialogueConfig();
			if (config.contains(dialogue_key) && config[dialogue_key].is_object())
				return config[dialogue_key];

			Core::Logger::errorLog("StoryTrigger: brak dialogue_key: " + dialogue_key);
			return {};
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
				const std::string entity_name = action.value("name", action.value("entity_name", ""));
				if (!entity_name.empty()) {
					for (const auto& entity : engine->getEntityManager().getEntities()) {
						if (entity && entity->getName() == entity_name)
							entity->setDormant(true);
					}
				}
			} else if (type == "play_entity_animation") {
				const std::string entity_name = action.value("name", action.value("entity_name", ""));
				const std::string animation = action.value("animation", "");
				if (!entity_name.empty() && !animation.empty()) {
					for (const auto& entity : engine->getEntityManager().getEntities()) {
						if (!entity || entity->getName() != entity_name)
							continue;

						if (action.value("face_player", false)) {
							if (const auto& player = engine->getPlayer())
								entity->rotateTowardsCenter(player->getCenter().x, player->getCenter().y);
						}

						entity->setAnimationSpeed(action.value("animation_speed", 1.0f));
						if (action.value("freeze_last_frame", false))
							entity->playAnimationFreezeOnLastFrame(animation, action.value("lock_movement", false), action.value("start_frame", 0), true);
						else
							entity->playAnimation(animation, action.value("loop", false), action.value("lock_movement", false), action.value("start_frame", 0), true);
						break;
					}
				}
			}
		}

		std::vector<nlohmann::json> readActionList(const nlohmann::json& data) {
			std::vector<nlohmann::json> actions;
			if (data.contains("actions") && data["actions"].is_array()) {
				for (const auto& action : data["actions"]) {
					if (action.is_object())
						actions.push_back(action);
				}
			}
			return actions;
		}

		Game::DialogueTree buildNodeDialogueTree(
			const nlohmann::json& dialogue_json,
			const std::function<void(const nlohmann::json&)>& execute_option_action
		) {
			Game::DialogueTree tree;
			const auto nodes_it = dialogue_json.find("nodes");
			if (nodes_it == dialogue_json.end() || !nodes_it->is_array())
				return tree;

			for (const auto& node_json : *nodes_it) {
				if (!node_json.is_object())
					continue;

				Game::DialogueNode node;
				node.id = node_json.value("id", static_cast<int>(tree.getNode(0) ? 1 : 0));
				node.speaker_name = node_json.value("speaker", "");
				node.text = node_json.value("text", "");
				node.voice_path = node_json.value("voice_path", "");

				if (node_json.contains("options") && node_json["options"].is_array()) {
					for (const auto& option_json : node_json["options"]) {
						if (!option_json.is_object())
							continue;

						Game::DialogueOption option;
						option.text = option_json.value("text", "Dalej");
						option.next_node_id = option_json.value("next_node_id", option_json.value("next", -1));

						std::vector<nlohmann::json> actions = readActionList(option_json);
						if (!actions.empty()) {
							option.action = [actions = std::move(actions), execute_option_action]() {
								for (const auto& action : actions)
									execute_option_action(action);
							};
						}

						node.options.push_back(std::move(option));
					}
				}

				if (node.options.empty()) {
					Game::DialogueOption option;
					option.text = node_json.value("option", "Rozumiem.");
					option.next_node_id = -1;
					node.options.push_back(option);
				}

				tree.addNode(node);
			}

			return tree;
		}

		Game::DialogueTree buildDialogueTree(
			const nlohmann::json& dialogue_json,
			const std::function<void(const nlohmann::json&)>& execute_option_action = nullptr
		) {
			Game::DialogueTree tree;
			if (!dialogue_json.is_object())
				return tree;

			if (dialogue_json.contains("nodes") && execute_option_action)
				return buildNodeDialogueTree(dialogue_json, execute_option_action);

			const auto lines_it = dialogue_json.find("lines");
			if (lines_it == dialogue_json.end() || !lines_it->is_array())
				return tree;

			const std::string final_option = dialogue_json.value("final_option", "Rozumiem.");
			for (size_t i = 0; i < lines_it->size(); ++i) {
				const auto& line = (*lines_it)[i];
				Game::DialogueNode node;
				node.id = static_cast<int>(i);
				node.speaker_name = line.value("speaker", "");
				node.text = line.value("text", "");
				node.voice_path = line.value("voice_path", "");

				Game::DialogueOption option;
				size_t next_line = i + 1;
				if (next_line < lines_it->size() && isPlayerDialogueSpeaker((*lines_it)[next_line].value("speaker", ""))) {
					option.text = (*lines_it)[next_line].value("text", "");
				} else {
					const bool is_final_node = next_line >= lines_it->size();
					option.text = is_final_node
						? resolveFinalOption(final_option, node.speaker_name, node.text)
						: "Dalej";
				}

				option.next_node_id = (next_line < lines_it->size()) ? static_cast<int>(next_line) : -1;
				node.options.push_back(option);
				tree.addNode(node);
			}

			return tree;
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

		std::shared_ptr<Entity> findEntityByName(Core::Engine* engine, const std::string& name) {
			if (!engine || name.empty())
				return nullptr;

			for (const auto& entity : engine->getEntityManager().getEntities()) {
				if (entity && entity->getName() == name)
					return entity;
			}
			return nullptr;
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

	}

	StoryTrigger::StoryTrigger(
		const std::string& name,
		const float x,
		const float y,
		const float width,
		const float height,
		Core::Engine* engine,
		nlohmann::json data
	)
		: InteractiveTrigger(name, x, y, nullptr, 1),
		  _engine(engine),
		  _data(std::move(data))
	{
		setType(EntityType::Trigger);
		setFaction(Faction::None);
		_once = _data.value("once", true);
		setCollider(std::make_unique<RectangleCollider>(this, width, height, 0.0f, 0.0f));
	}

	void StoryTrigger::onTriggerEnter(Entity& other) {
		if (isDormant() || _dialogue_open)
			return;

		if (_once && _completed)
			return;

		if (other.getFaction() != Faction::Player)
			return;

		if (!Game::areEntityConditionsMet(_data, _engine))
			return;

		for (const auto& action : collectEnterActions(_data))
			executeActionData(_engine, this, action);

		run(_engine);
	}

	void StoryTrigger::run(Core::Engine* engine) {
		if (!engine)
			return;

		engine->cancelPlayerAction();

		const nlohmann::json dialogue_json = resolveDialogueJson(_data);
		if (!dialogue_json.is_object()) {
			executeActions(engine);
			return;
		}

		Game::DialogueTree tree = buildDialogueTree(dialogue_json, [this, engine](const nlohmann::json& action) {
			executeActionData(engine, this, action);
		});
		if (!tree.getNode(0)) {
			executeActions(engine);
			return;
		}

		_dialogue_open = true;
		engine->getUIHandler().openDialogueFacing(tree, resolveDialogueSpeaker(engine, _data), 0, [this, engine](const int, const bool completed) {
			_dialogue_open = false;
			if (!completed)
				return;

			executeActions(engine);
		});
	}

	void StoryTrigger::executeActions(Core::Engine* engine) {
		if (!engine)
			return;

		for (const auto& action : collectActions(_data)) {
			executeActionData(engine, this, action);
		}

		_completed = true;
		if (_once)
			setDormant(true);
	}

	void StoryTrigger::render(const Camera3D& camera) {
		if (DebugColliders) {
			auto* rect_collider = dynamic_cast<RectangleCollider*>(getCollider());
			if (!rect_collider)
				return;

			const Vector2 center = rect_collider->getPosition();
			const float width = rect_collider->getWidth();
			const float height = rect_collider->getHeight();
			DrawCubeWires(Vector3{center.x, getAltitude() + 0.1f, center.y}, width, 0.2f, height, SKYBLUE);

			const Vector2 screen_pos = GetWorldToScreen(Vector3{center.x, getAltitude() + 0.6f, center.y}, camera);
			DrawText(getName().c_str(), static_cast<int>(screen_pos.x - 35), static_cast<int>(screen_pos.y - 10), 10, SKYBLUE);
		}
	}

	float StoryTrigger::getInteractionRange() {
		return 0.0f;
	}

	nlohmann::json StoryTrigger::serializeState() const {
		nlohmann::json state = Entity::serializeState();
		state["completed"] = _completed;
		state["once"] = _once;
		return state;
	}

	void StoryTrigger::applyState(const nlohmann::json& state, Item::ItemDatabase* item_database) {
		Entity::applyState(state, item_database);
		if (!state.is_object())
			return;

		_completed = state.value("completed", _completed);
		_once = state.value("once", _once);
		if (_completed && _once)
			setDormant(true);
	}

} // namespace Nawia::Entity
