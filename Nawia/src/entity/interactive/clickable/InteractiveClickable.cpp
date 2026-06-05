#include "InteractiveClickable.h"

#include <Engine.h>
#include <UIHandler.h>

namespace Nawia::Entity {

	void InteractiveClickable::onInteractionCompleted(Entity& instigator, Core::Engine& engine) {
		(void)instigator;

		if (getInventory())
			engine.getUIHandler().openContainer(this);
	}

} // namespace Nawia::Entity
