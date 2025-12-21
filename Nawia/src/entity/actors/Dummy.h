#pragma once
#include "Enemy.h"

#include <Map.h>

namespace Nawia::Entity {

	class Dummy : public Enemy 
	{
	public:
		Dummy(float x, float y, const std::shared_ptr<SDL_Texture>& tex, int max_hp, Core::Map* map);

		void update(float dt) override;

	private:
		float _target_x, _target_y;
		float _stay_timer;

		void pickNewTarget();
	};

} // namespace Nawia::Entity
