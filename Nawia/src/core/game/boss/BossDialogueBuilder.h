#pragma once

#include <Dialogue.h>

#include <string>

namespace Nawia::Game {

	class BossDialogueBuilder {
	public:
		[[nodiscard]] static DialogueTree buildFromNpcConfig(const std::string& dialogue_key);
	};

} // namespace Nawia::Game
