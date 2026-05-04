#pragma once
#include <Entity.h>
#include <Interactable.h>
#include <Collider.h>

namespace Nawia::Entity {

	/**
	 * @class InteractiveTrigger
	 * @brief Baza obszarów aktywowanych kolizją z inną encją.
	 */
	class InteractiveTrigger : public Entity, public Interactable {
	public:
		using Entity::Entity;

		/**
		 * @brief Triggery nie obsługują bezpośredniego kliknięcia.
		 */
		void onInteract(Entity& instigator) override {}
		float getInteractionRange() override { return 1.0f; }
	};

} // namespace Nawia::Entity
