#pragma once

#include <Entity.h>

namespace Nawia::Entity {

	class MiniMushroomProp : public Entity {
	public:
		MiniMushroomProp();

		void update(float delta_time) override;

	private:
		bool _jumping = false;
		float _jump_timer = 2.0f;
	};

} // namespace Nawia::Entity
