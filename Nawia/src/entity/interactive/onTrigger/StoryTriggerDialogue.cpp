#include "StoryTriggerInternal.h"

#include <DialogueJson.h>
#include <Logger.h>

#include <string>

namespace Nawia::Entity::StoryTriggerSupport {

	namespace {

	}

	nlohmann::json resolveDialogueJson(const nlohmann::json& data) {
		if (data.contains("dialogue") && data["dialogue"].is_object())
			return data["dialogue"];

		const std::string dialogue_key = data.value("dialogue_key", "");
		if (dialogue_key.empty())
			return {};

		if (const auto* dialogue = Game::DialogueJson::findNpcDialogue(dialogue_key))
			return *dialogue;

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
			return Game::DialogueJson::buildJsonActionTree(dialogue_json, execute_option_action);

		return Game::DialogueJson::buildLinearTree(dialogue_json);
	}

} // namespace Nawia::Entity::StoryTriggerSupport
