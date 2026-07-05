#include "BossDialogueBuilder.h"

#include <Logger.h>
#include <json.hpp>

#include <fstream>

namespace Nawia::Game {

	namespace {

		nlohmann::json loadJsonDocument(const std::string& path)
		{
			std::ifstream file(path);
			if (!file.is_open()) {
				Core::Logger::errorLog("BossManager: nie mozna otworzyc JSON: " + path);
				return {};
			}

			nlohmann::json data;
			try {
				file >> data;
			} catch (const nlohmann::json::parse_error&) {
				Core::Logger::errorLog("BossManager: blad parsowania JSON: " + path);
				return {};
			}
			return data;
		}

		bool isPlayerDialogueSpeaker(const std::string& speaker)
		{
			return speaker == "Logos" || speaker == "Jarko" || speaker == "Player" || speaker == "Gracz";
		}

		bool isPlaceholderOption(const std::string& text)
		{
			return text.empty() || text == "..." || text == "Dalej";
		}

		std::string resolveFinalOption(
			const std::string& configured_text,
			const std::string& current_speaker,
			const std::string& current_text)
		{
			if (!isPlaceholderOption(configured_text))
				return configured_text;

			return isPlayerDialogueSpeaker(current_speaker) ? current_text : "Rozumiem.";
		}

	} // namespace

	DialogueTree BossDialogueBuilder::buildFromNpcConfig(const std::string& dialogue_key)
	{
		DialogueTree tree;
		static const nlohmann::json config = loadJsonDocument("assets/data/npc_dialogues.json");
		if (dialogue_key.empty() || !config.contains(dialogue_key) || !config[dialogue_key].is_object())
			return tree;

		const auto& dialogue_json = config[dialogue_key];
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

} // namespace Nawia::Game
