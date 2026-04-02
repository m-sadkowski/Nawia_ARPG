#pragma once

#include "Entity.h"

namespace Nawia::Entity {

	/**
	 * @class StaticObject
	 * @brief Represents static environmental objects like trees, rocks.
	 * They do not move, and have a base texture and collision box.
	 */
	class StaticObject : public Entity {
	public:
		StaticObject();
		~StaticObject() override = default;

		void update(float delta_time) override;
		void render(const Camera3D& camera) override;
	};

	class StaticObjectBuilder : public EntityBuilder<StaticObjectBuilder> {
	public:
		StaticObjectBuilder() {
			_static_object_ptr = std::shared_ptr<StaticObject>(new StaticObject());
			this->_entity = _static_object_ptr.get();
		}

		std::shared_ptr<StaticObject> build() {
			return _static_object_ptr;
		}
	private:
		std::shared_ptr<StaticObject> _static_object_ptr;
	};

} // namespace Nawia::Entity
