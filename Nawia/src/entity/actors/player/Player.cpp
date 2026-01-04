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
		addAnimation("attack", "../assets/models/player_auto_attack.glb");
		playAnimation("default"); // play idle

		// add collider
		setCollider(std::make_unique<RectangleCollider>(this, 0.3f, 0.8f, -2.1f, -1.f));
	}

	void Player::moveTo(const float x, const float y)
	{
		// Adjusted target so that the CENTER of the collider lands on the click point, not the feet.
		const Vector2 center = getCenter();
		const float offset_x = center.x - getX();
		const float offset_y = center.y - getY();

		_target_x = x - offset_x;
		_target_y = y - offset_y;
		_is_moving = true;

		if (!isAnimationLocked())
			playAnimation("walk");

		const float dx = _target_x - getX();
		const float dy = _target_y - getY();
		const float distance_sq = dx * dx + dy * dy;

		if (distance_sq > 0.001f)
		{
			rotateTowards(_target_x, _target_y);
		}
	}

	void Player::stop()
	{
		_is_moving = false;
		if (!isAnimationLocked())
		{
			playAnimation("default");
		}
	}

	void Player::update(const float delta_time)
	{
		Entity::update(delta_time);
		updateAbilities(delta_time);
		updateMovement(delta_time);
	}

	void Player::updateMovement(const float delta_time)
	{
		if (!_is_moving)
			return;

		const float dx = _target_x - _pos.x;
		const float dy = _target_y - _pos.y;
		const float distance = std::sqrt(dx * dx + dy * dy);

		if (!isAnimationLocked())
			playAnimation("walk");

		if (distance < 0.1f)
		{
			_pos.x = _target_x;
			_pos.y = _target_y;
			_is_moving = false;

			if (!isAnimationLocked())
				playAnimation("default");
		}
		else
		{
			_pos.x += (dx / distance) * _speed * delta_time;
			_pos.y += (dy / distance) * _speed * delta_time;
		}
	}

	


} // namespace Nawia::Entity