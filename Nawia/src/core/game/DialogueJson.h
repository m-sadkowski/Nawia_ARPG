#pragma once

#include <Dialogue.h>

#include <functional>
#include <json.hpp>
#include <string>

namespace Nawia::Game::DialogueJson {

	[[nodiscard]] const nlohmann::json& getNpcDialogueConfig();
	[[nodiscard]] const nlohmann::json* findNpcDialogue(const std::string& key);
	[[nodiscard]] bool isPlayerSpeaker(const std::string& speaker);
	[[nodiscard]] std::string resolveFinalOption(
		const std::string& configured_text,
		const std::string& current_speaker,
		const std::string& current_text
	);
	[[nodiscard]] DialogueTree buildLinearTree(const nlohmann::json& dialogue_json);
	[[nodiscard]] DialogueTree buildStringActionTree(
		const nlohmann::json& dialogue_json,
		const std::function<void(const std::string&)>& execute_option_action,
		const std::string& default_option_text = "Dalej"
	);
	[[nodiscard]] DialogueTree buildJsonActionTree(
		const nlohmann::json& dialogue_json,
		const std::function<void(const nlohmann::json&)>& execute_option_action,
		const std::string& default_option_text = "Dalej"
	);

} // namespace Nawia::Game::DialogueJson
