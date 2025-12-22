#pragma once
#include "EnemyInterface.h"

#include <Map.h>

namespace Nawia::Entity {

	class Dummy : public EnemyInterface {
	public:
		Dummy(float x, float y, const std::shared_ptr<Texture2D>& tex, int max_hp, Core::Map* map);

		void update(float dt) override;

	private:
		float _target_x, _target_y;
		float _stay_timer;

		void pickNewTarget();
	};

} // namespace Nawia::Entity
