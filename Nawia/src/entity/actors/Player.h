#pragma once
#include "Entity.h"

namespace Nawia::Entity {

	class Player : public Entity {
	public:
		Player(float x, float y, const std::shared_ptr<SDL_Texture>& texture);

		void update(float delta_time) override;

		void moveTo(float x, float y);

	private:
		float _target_x, _target_y;
		float _speed;
		bool _is_moving;
	};

} // namespace Nawia::Entity