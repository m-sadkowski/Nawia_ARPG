#pragma once
#include <memory>

namespace Nawia::Core {
	class Engine;
}
namespace Nawia::Entity {
	class Cat;
}

namespace Nawia::Game {
	class DialogueManager {
	public:
		void createCatDialogue(Core::Engine* engine, std::shared_ptr<Entity::Cat> cat);
	};
}