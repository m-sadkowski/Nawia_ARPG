#include "StaticObject.h"
#include <Collider.h>

namespace Nawia::Entity {

	StaticObject::StaticObject() {
		setType(EntityType::NPCStatic);
		setFaction(Faction::Neutral);
	}

	void StaticObject::update(float delta_time) {
		Entity::update(delta_time);
	}

	void StaticObject::render(const Camera3D& camera) {
		Entity::render(camera);
	}

} // namespace Nawia::Entity
