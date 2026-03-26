#include "StaticObject.h"
#include "Collider.h"

namespace Nawia::Entity {

	StaticObject::StaticObject() {
		_type = EntityType::NPCStatic; 
		setFaction(Faction::Neutral);
		_use_3d_rendering = false; 

		// We assume 3D model isn't used for simple trees unless loaded
	}

	void StaticObject::update(float delta_time) {
		Entity::update(delta_time);
	}

	void StaticObject::render(float offset_x, float offset_y) {
		Entity::render(offset_x, offset_y);
	}

} // namespace Nawia::Entity
