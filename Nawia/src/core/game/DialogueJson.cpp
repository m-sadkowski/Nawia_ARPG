#include "DialogueJson.h"

#include <Logger.h>

#include <fstream>
#include <utility>
#include <vector>

namespace Nawia::Game::DialogueJson {

	namespace {
		nlohmann::json loadJsonDocument(const std::string& path) {
			std::ifstream file(path);
			if (!file.is_open()) {
				Nawia::Core::Logger::errorLog("DialogueJson: nie mozna otworzyc JSON: " + path);
				return {};
			}

			nlohmann::json data;
			try {
				file >> data;
			} catch (const nlohmann::json::parse_error&) {
				Nawia::Core::Logger::errorLog("DialogueJson: blad parsowania JSON: " + path);
				return {};
			}
			return data;
		}

		bool isPlaceholderOption(const std::string& text) {
			return text.empty() || text == "..." || text == "Dalej";
		}

		std::vector<std::string> readStringActions(const nlohmann::json& option_json) {
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

		std::vector<nlohmann::json> readObjectActions(const nlohmann::json& option_json) {
			std::vector<nlohmann::json> actions;
			if (option_json.contains("actions") && option_json["actions"].is_array()) {
				for (const auto& action_json : option_json["actions"]) {
					if (action_json.is_object())
						actions.push_back(action_json);
				}
			}
			return actions;
		}

		DialogueTree buildNodeTree(
			const nlohmann::json& dialogue_json,
			const std::string& default_option_text,
			const std::function<void(DialogueOption&, const nlohmann::json&)>& bind_option_action
		) {
			DialogueTree tree;
			const auto nodes_it = dialogue_json.find("nodes");
			if (nodes_it == dialogue_json.end() || !nodes_it->is_array())
				return tree;

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
						option.text = option_json.value("text", default_option_text);
						option.next_node_id = option_json.value("next_node_id", option_json.value("next", -1));
						bind_option_action(option, option_json);
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

	const nlohmann::json& getNpcDialogueConfig() {
		static const nlohmann::json config = loadJsonDocument("assets/data/npc_dialogues.json");
		return config;
	}

	const nlohmann::json* findNpcDialogue(const std::string& key) {
		const auto& config = getNpcDialogueConfig();
		if (!config.contains(key) || !config[key].is_object())
			return nullptr;

		return &config[key];
	}

	bool isPlayerSpeaker(const std::string& speaker) {
		return speaker == "Logos" || speaker == "Jarko" || speaker == "Player" || speaker == "Gracz";
	}

	std::string resolveFinalOption(
		const std::string& configured_text,
		const std::string& current_speaker,
		const std::string& current_text
	) {
		if (!isPlaceholderOption(configured_text))
			return configured_text;

		return isPlayerSpeaker(current_speaker) ? current_text : "Rozumiem.";
	}

	DialogueTree buildLinearTree(const nlohmann::json& dialogue_json) {
		DialogueTree tree;
		if (!dialogue_json.is_object())
			return tree;

		const auto lines_it = dialogue_json.find("lines");
		if (lines_it == dialogue_json.end() || !lines_it->is_array())
			return tree;

		const std::string final_option = dialogue_json.value("final_option", "Rozumiem.");
		for (size_t i = 0; i < lines_it->size(); ++i) {
			const auto& line = (*lines_it)[i];
			DialogueNode node;
			node.id = static_cast<int>(i);
			node.speaker_name = line.value("speaker", "");
			node.text = line.value("text", "");
			node.voice_path = line.value("voice_path", "");

			DialogueOption option;
			const size_t next_line = i + 1;
			if (next_line < lines_it->size() && isPlayerSpeaker((*lines_it)[next_line].value("speaker", ""))) {
				option.text = (*lines_it)[next_line].value("text", "");
			} else {
				const bool is_final_node = next_line >= lines_it->size();
				option.text = is_final_node
					? resolveFinalOption(final_option, node.speaker_name, node.text)
					: "Dalej";
			}

			option.next_node_id = (next_line < lines_it->size()) ? static_cast<int>(next_line) : -1;
			node.options.push_back(std::move(option));
			tree.addNode(node);
		}

		return tree;
	}

	DialogueTree buildStringActionTree(
		const nlohmann::json& dialogue_json,
		const std::function<void(const std::string&)>& execute_option_action,
		const std::string& default_option_text
	) {
		return buildNodeTree(dialogue_json, default_option_text, [execute_option_action](DialogueOption& option, const nlohmann::json& option_json) {
			auto actions = readStringActions(option_json);
			if (actions.empty() || !execute_option_action)
				return;

			option.action = [actions = std::move(actions), execute_option_action]() {
				for (const auto& action : actions)
					execute_option_action(action);
			};
		});
	}

	DialogueTree buildJsonActionTree(
		const nlohmann::json& dialogue_json,
		const std::function<void(const nlohmann::json&)>& execute_option_action,
		const std::string& default_option_text
	) {
		return buildNodeTree(dialogue_json, default_option_text, [execute_option_action](DialogueOption& option, const nlohmann::json& option_json) {
			auto actions = readObjectActions(option_json);
			if (actions.empty() || !execute_option_action)
				return;

			option.action = [actions = std::move(actions), execute_option_action]() {
				for (const auto& action : actions)
					execute_option_action(action);
			};
		});
	}

} // namespace Nawia::Game::DialogueJson
