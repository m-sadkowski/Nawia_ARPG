#pragma once

#include <Entity.h>
#include <Interactable.h>

namespace Nawia::Item {
	class Backpack;
}

namespace Nawia::Entity {

	/**
	 * @class InteractiveClickable
	 * @brief Baza obiektów obsługiwanych kliknięciem zamiast wejściem w trigger.
	 */
	class InteractiveClickable : public Entity, public Interactable {
	public:
		using Entity::Entity;

		/**
		 * @brief Klikalne obiekty ignorują wejście w trigger.
		 */
		void onTriggerEnter(Nawia::Entity::Entity& other) override {}

		/**
		 * @brief Zwraca ekwipunek obiektu, jeśli dany obiekt go udostępnia.
		 */
		virtual Item::Backpack* getInventory() { return nullptr; }
	};

} // namespace Nawia::Entity
