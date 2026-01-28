#pragma once
#include <memory>

namespace Nawia::Core { class Engine; }

namespace Nawia::Entity { class Cat; }

namespace Nawia::Game {

	class DialogueManager {
	public:
		void createCatDialogue(Core::Engine* engine, Entity::Cat* cat);
		void createCatQuestCompletedDialogue(Core::Engine* engine, Entity::Cat* cat);
	};

} // namespace Nawia::Game