#include "BossDialogueBuilder.h"

#include <DialogueJson.h>

namespace Nawia::Game {

	DialogueTree BossDialogueBuilder::buildFromNpcConfig(const std::string& dialogue_key)
	{
		const auto* dialogue = DialogueJson::findNpcDialogue(dialogue_key);
		return dialogue ? DialogueJson::buildLinearTree(*dialogue) : DialogueTree{};
	}

} // namespace Nawia::Game
