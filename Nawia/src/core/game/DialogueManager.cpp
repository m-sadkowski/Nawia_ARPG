#include "DialogueManager.h"

#include <Cat.h>
#include <Dialogue.h>
#include <DialogueJson.h>
#include <Engine.h>

#include <functional>
#include <string>

namespace Nawia::Game {
	namespace {
		DialogueTree buildDialogueFromConfig(
			const std::string& key,
			const std::function<void(const std::string&)>& execute_option_action) {
			const auto* dialogue = DialogueJson::findNpcDialogue(key);
			return dialogue ? DialogueJson::buildStringActionTree(*dialogue, execute_option_action, "Dalej.") : DialogueTree{};
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
