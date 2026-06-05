#include "StoryTrigger.h"

#include <BossManager.h>
#include <Collider.h>
#include <Dialogue.h>
#include <Engine.h>
#include <Level.h>
#include <LevelManager.h>
#include <Logger.h>
#include <QuestManager.h>
#include <StoryConditions.h>
#include <UIHandler.h>

#include <fstream>
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

		Game::DialogueTree buildDialogueTree(const nlohmann::json& dialogue_json) {
			Game::DialogueTree tree;
			if (!dialogue_json.is_object())
				return tree;

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
					next_line++;
				} else {
					const bool is_final_node = next_line >= lines_it->size();
					option.text = is_final_node
						? resolveFinalOption(final_option, node.speaker_name, node.text)
						: (isPlayerDialogueSpeaker(node.speaker_name) ? node.text : "Rozumiem.");
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

		Game::DialogueTree tree = buildDialogueTree(dialogue_json);
		if (!tree.getNode(0)) {
			executeActions(engine);
			return;
		}

		_dialogue_open = true;
		engine->getUIHandler().openDialogue(tree, 0, [this, engine](const int, const bool completed) {
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
			if (!Game::areStoryConditionsMet(action.value("conditions", nlohmann::json::object()), engine))
				continue;

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
				if (!quest_id.empty())
					engine->getQuestManager().completeQuest(quest_id, engine);
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
				if (!boss_id.empty())
					engine->getBossManager().startBossFight(boss_id, engine, getCenter(), getAltitude());
			}
		}

		_completed = true;
		if (_once)
			setDormant(true);
	}

	void StoryTrigger::render(const Camera3D& camera) {
		if (DebugColliders && _collider) {
			auto* rect_collider = dynamic_cast<RectangleCollider*>(_collider.get());
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

} // namespace Nawia::Entity
