#include "Player.h"

#include <Constants.h>
#include <MathUtils.h>

#include <cmath>

namespace Nawia::Entity {

	Player::Player(const float x, const float y, const std::shared_ptr<Texture2D>& texture)
	    : Entity(x, y, texture, 100), _target_x(x), _target_y(y), _speed(4.0f), _is_moving(false) 
	{
		loadModel("../assets/models/player.glb");
		addAnimation("walk", "../assets/models/player_walk.glb");
		playAnimation("default"); // play idle
	}

	void Player::moveTo(const float x, const float y)
	{
		_target_x = x;
		_target_y = y;
		_is_moving = true;

		playAnimation("walk");

		const float dx = _target_x - getX();
		const float dy = _target_y - getY();
		const float distance_sq = dx * dx + dy * dy;

		if (distance_sq > 0.001f)
		{
			const float iso_dx = (dx - dy) * (Core::TILE_WIDTH / 2.0f);
			const float iso_dy = (dx + dy) * (Core::TILE_HEIGHT / 2.0f);
			const float screen_angle = std::atan2(iso_dy, iso_dx) * 180.0f / static_cast<float>(Core::pi);
			setRotation(90.0f - screen_angle);
		}
	}

	void Player::update(const float delta_time)
	{
		Entity::update(delta_time);
		updateAbilities(delta_time);

		if (!_is_moving)
			return;

		const float dx = _target_x - _pos->getX();
		const float dy = _target_y - _pos->getY();
		const float distance = std::sqrt(dx * dx + dy * dy);

		if (distance < 0.1f)
		{
			_pos->setX(_target_x);
			_pos->setY(_target_y);
			_is_moving = false;

			playAnimation("default");
		}
		else
		{
			_pos->setX(_pos->getX() + (dx / distance) * _speed * delta_time);
			_pos->setY(_pos->getY() + (dy / distance) * _speed * delta_time);
		}
	}

	


} // namespace Nawia::Entity