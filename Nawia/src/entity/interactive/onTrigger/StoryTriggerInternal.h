#pragma once

#include <Dialogue.h>

#include <functional>
#include <json.hpp>
#include <memory>
#include <vector>

namespace Nawia::Core { class Engine; }

namespace Nawia::Entity {
	class Entity;
	class StoryTrigger;
}

namespace Nawia::Entity::StoryTriggerSupport {

	nlohmann::json resolveDialogueJson(const nlohmann::json& data);
	Game::DialogueTree buildDialogueTree(
		const nlohmann::json& dialogue_json,
		const std::function<void(const nlohmann::json&)>& execute_option_action = nullptr
	);

	std::vector<nlohmann::json> collectActions(const nlohmann::json& data);
	std::vector<nlohmann::json> collectEnterActions(const nlohmann::json& data);
	void executeActionData(Core::Engine* engine, StoryTrigger* trigger, const nlohmann::json& action);
	std::shared_ptr<Entity> resolveDialogueSpeaker(Core::Engine* engine, const nlohmann::json& data);

} // namespace Nawia::Entity::StoryTriggerSupport
