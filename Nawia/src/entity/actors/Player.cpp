#include "Player.h"

namespace Nawia::Entity {

	Player::Player(float x, float y, std::shared_ptr<SDL_Texture> texture)
		: Entity(x, y, texture), _targetX(x), _targetY(y), _speed(4.0f), _isMoving(false) {

	}

	void Player::moveTo(float x, float y) {
		_targetX = x;
		_targetY = y;
		_isMoving = true;
	}

	void Player::update(float deltaTime) {
		if (!_isMoving) {
			return;
		}

		float dx = _targetX - _pos->getX();
		float dy = _targetY - _pos->getY();

		float distance = std::sqrt(dx * dx + dy * dy);

		if (distance < 0.1f) {
			_pos->setX(_targetX);
			_pos->setY(_targetY);
			_isMoving = false;
		}
		else {
			_pos->setX(_pos->getX() + (dx / distance) * _speed * deltaTime);
			_pos->setY(_pos->getY() + (dy / distance) * _speed * deltaTime);
 		}
	}

}