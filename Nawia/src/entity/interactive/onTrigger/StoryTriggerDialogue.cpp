#include "StoryTriggerInternal.h"

#include <Logger.h>

#include <fstream>
#include <string>

namespace Nawia::Entity::StoryTriggerSupport {

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

	Game::DialogueTree buildDialogueTree(
		const nlohmann::json& dialogue_json,
		const std::function<void(const nlohmann::json&)>& execute_option_action
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

} // namespace Nawia::Entity::StoryTriggerSupport
