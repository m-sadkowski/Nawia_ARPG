#pragma once

namespace Nawia::Core { class Engine; }
namespace Nawia::Entity { class Cat; }

namespace Nawia::Game {

	/**
	 * @class DialogueManager
	 * @brief Buduje drzewa dialogow dla NPC.
	 *
	 * Manager nie posiada przekazanych obiektow. `Engine*` i `Cat*` sa
	 * krotkozyjacymi widokami na obiekty zarzadzane przez silnik i EntityManager.
	 */
	class DialogueManager {
	public:
		/**
		 * @brief Tworzy poczatkowy dialog kota z dostepem do jego ekwipunku.
		 */
		void createCatDialogue(Core::Engine* engine, Entity::Cat* cat);

		/**
		 * @brief Tworzy dialog kota po ukonczeniu questa z ryba.
		 */
		void createCatQuestCompletedDialogue(Core::Engine* engine, Entity::Cat* cat);
	};

} // namespace Nawia::Game
