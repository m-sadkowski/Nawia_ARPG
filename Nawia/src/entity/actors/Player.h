#pragma once
#include "Entity.h"

namespace Nawia::Entity {

	class Player : public Entity {
	public:
		Player(float x, float y, std::shared_ptr<SDL_Texture> texture);

		void update(float deltaTime) override;

		void moveTo(float x, float y);

	private:
		float _targetX, _targetY;
		float _speed;
		bool _isMoving;
	};

}