#include "DialogueManager.h"

#include <Cat.h>
#include <Dialogue.h>
#include <Engine.h>
#include <Logger.h>

#include <fstream>
#include <functional>
#include <json.hpp>
#include <string>
#include <utility>
#include <vector>

namespace Nawia::Game {
	namespace {
		nlohmann::json loadJsonDocument(const std::string& path) {
			std::ifstream file(path);
			if (!file.is_open()) {
				Nawia::Core::Logger::errorLog("DialogueManager: nie mozna otworzyc JSON: " + path);
				return {};
			}

			nlohmann::json data;
			try {
				file >> data;
			} catch (const nlohmann::json::parse_error&) {
				Nawia::Core::Logger::errorLog("DialogueManager: blad parsowania JSON: " + path);
				return {};
			}
			return data;
		}

		const nlohmann::json& getNpcDialogueConfig() {
			static const nlohmann::json config = loadJsonDocument("assets/data/npc_dialogues.json");
			return config;
		}

		std::vector<std::string> readActionIds(const nlohmann::json& option_json) {
			std::vector<std::string> actions;
			if (option_json.contains("action") && option_json["action"].is_string())
				actions.push_back(option_json["action"].get<std::string>());

			if (option_json.contains("actions") && option_json["actions"].is_array()) {
				for (const auto& action_json : option_json["actions"]) {
					if (action_json.is_string())
						actions.push_back(action_json.get<std::string>());
				}
			}

			return actions;
		}

		DialogueTree buildDialogueFromConfig(
			const std::string& key,
			const std::function<void(const std::string&)>& execute_option_action) {
			const auto& config = getNpcDialogueConfig();
			if (!config.contains(key) || !config[key].is_object())
				return {};

			const auto nodes_it = config[key].find("nodes");
			if (nodes_it == config[key].end() || !nodes_it->is_array())
				return {};

			DialogueTree tree;
			for (const auto& node_json : *nodes_it) {
				if (!node_json.is_object())
					continue;

				DialogueNode node;
				node.id = node_json.value("id", 0);
				node.speaker_name = node_json.value("speaker", "");
				node.text = node_json.value("text", "");
				node.voice_path = node_json.value("voice_path", "");

				if (node_json.contains("options") && node_json["options"].is_array()) {
					for (const auto& option_json : node_json["options"]) {
						if (!option_json.is_object())
							continue;

						DialogueOption option;
						option.text = option_json.value("text", "Dalej.");
						option.next_node_id = option_json.value("next_node_id", option_json.value("next", -1));

						auto actions = readActionIds(option_json);
						if (!actions.empty() && execute_option_action) {
							option.action = [actions = std::move(actions), execute_option_action]() {
								for (const auto& action : actions)
									execute_option_action(action);
							};
						}

						node.options.push_back(std::move(option));
					}
				}

				if (node.options.empty()) {
					DialogueOption option;
					option.text = node_json.value("option", "Rozumiem.");
					option.next_node_id = -1;
					node.options.push_back(std::move(option));
				}

				tree.addNode(node);
			}

			return tree;
		}
	}

	void DialogueManager::createCatDialogue(Core::Engine* engine, Entity::Cat* cat) {
		if (!engine || !cat)
			return;

		cat->setDialogue(buildDialogueFromConfig("cat_intro", [engine, cat](const std::string& action) {
			if (action == "open_cat_container") {
				if (!engine || !cat)
					return;

				engine->getUIHandler().closeDialogue();
				engine->getUIHandler().openContainer(cat);
			}
		}));
	}

	void DialogueManager::createCatQuestCompletedDialogue(Core::Engine* engine, Entity::Cat* cat) {
		if (!engine || !cat)
			return;

		cat->setDialogue(buildDialogueFromConfig("cat_quest_completed", [engine, cat](const std::string& action) {
			if (action == "open_cat_container") {
				if (!engine || !cat)
					return;

				engine->getUIHandler().closeDialogue();
				engine->getUIHandler().openContainer(cat);
			}
		}));
	}

} // namespace Nawia::Game
