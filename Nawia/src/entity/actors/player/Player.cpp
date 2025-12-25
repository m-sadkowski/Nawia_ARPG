#include "Player.h"
#include "Collider.h"

#include <Constants.h>
#include <MathUtils.h>

#include <cmath>

namespace Nawia::Entity {

	Player::Player(const float x, const float y, const std::shared_ptr<Texture2D>& texture)
	    : Entity("Player", x, y, texture, 100), _target_x(x), _target_y(y), _speed(4.0f), _is_moving(false) 
	{
		this->setScale(0.03f);
		setFaction(Faction::Player);
		loadModel("../assets/models/player_idle.glb");
		addAnimation("walk", "../assets/models/player_walk.glb");
		addAnimation("attack", "../assets/models/player_attack.glb");
		playAnimation("default"); // play idle

		// add Collider
		setCollider(std::make_unique<RectangleCollider>(this, 0.5f, 0.5f));
	}

	void Player::moveTo(const float x, const float y)
	{
		if (isAnimationLocked())
			return;

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
			const float screen_angle = std::atan2(iso_dy, iso_dx) * 180.0f / PI;
			setRotation(90.0f - screen_angle);
		}
	}

	void Player::update(const float delta_time)
	{
		Entity::update(delta_time);
		updateAbilities(delta_time);

		if (!_is_moving || isAnimationLocked())
			return;

		const float dx = _target_x - _pos.x;
		const float dy = _target_y - _pos.y;
		const float distance = std::sqrt(dx * dx + dy * dy);

		playAnimation("walk");

		if (distance < 0.1f)
		{
			_pos.x = _target_x;
			_pos.y = _target_y;
			_is_moving = false;

			playAnimation("default");
		}
		else
		{
			_pos.x += (dx / distance) * _speed * delta_time;
			_pos.y += (dy / distance) * _speed * delta_time;
		}
	}

	


} // namespace Nawia::Entity