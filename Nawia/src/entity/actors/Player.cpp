#include "Player.h"

#include <Constants.h>
#include <MathUtils.h>

#include <cmath>

namespace Nawia::Entity {

	Player::Player(const float x, const float y, const std::shared_ptr<Texture2D>& texture)
	    : Entity(x, y, texture, 100), _target_x(x), _target_y(y), _speed(4.0f), _is_moving(false) 
	{
		_animation_component = std::make_unique<AnimationComponent>("../assets/models/player.glb");
		_animation_component->addAnimation("../assets/models/player_walk.glb");
		_animation_component->playAnimation(0); // play idle
	}

	void Player::moveTo(const float x, const float y)
	{
		_target_x = x;
		_target_y = y;
		_is_moving = true;

		if (_animation_component)
		{
			_animation_component->playAnimation(1);

			const float dx = _target_x - getX();
			const float dy = _target_y - getY();
			const float distance_sq = dx * dx + dy * dy;

			if (distance_sq > 0.001f)
			{
				const float iso_dx = (dx - dy) * (Core::TILE_WIDTH / 2.0f);
				const float iso_dy = (dx + dy) * (Core::TILE_HEIGHT / 2.0f);
				const float screen_angle = std::atan2(iso_dy, iso_dx) * 180.0f / static_cast<float>(Core::pi);
				_animation_component->setRotation(90.0f - screen_angle);
			}
		}
	}

	void Player::update(const float delta_time)
	{
		if (_animation_component)
			_animation_component->update(delta_time);

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

			if (_animation_component)
				_animation_component->playAnimation(0);
		}
		else
		{
			_pos->setX(_pos->getX() + (dx / distance) * _speed * delta_time);
			_pos->setY(_pos->getY() + (dy / distance) * _speed * delta_time);
		}
	}

	void Player::render(const float offset_x, const float offset_y) {
		Core::Point2D pos = getScreenPos(getX(), getY(), offset_x, offset_y);

		if (_animation_component) {
			const Vector2 screen_pos = { pos.getX(), pos.getY() };
			constexpr float model_scale = 3.0f;
			_animation_component->render(screen_pos, model_scale);
		}
		else {
			Entity::render(offset_x, offset_y);
		}
	}

	void Player::addAbility(const std::shared_ptr<Ability>& ability) 
	{
		_abilities.push_back(ability);
	}

	std::shared_ptr<Ability> Player::getAbility(const int index)
	{
		if (index >= 0 && index < _abilities.size())
			return _abilities[index];

		return nullptr;
	}

	void Player::updateAbilities(const float dt) const 
	{
		for (auto &s : _abilities)
			s->update(dt);
	}

} // namespace Nawia::Entity