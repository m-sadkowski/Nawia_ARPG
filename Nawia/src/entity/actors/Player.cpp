#include "Player.h"

namespace Nawia::Entity 
{

	Player::Player(const float x, const float y, const std::shared_ptr<SDL_Texture>& texture)
		: Entity(x, y, texture, 200), _target_x(x), _target_y(y), _speed(4.0f), _is_moving(false) {}

	void Player::moveTo(const float x, const float y) 
	{
		_target_x = x;
		_target_y = y;
		_is_moving = true;
	}

	void Player::update(const float delta_time) 
	{
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
		}
		else 
		{
			_pos->setX(_pos->getX() + (dx / distance) * _speed * delta_time);
			_pos->setY(_pos->getY() + (dy / distance) * _speed * delta_time);
 		}
	}

	void Player::addSpell(const std::shared_ptr<Core::Spell>& spell) 
	{
		_spellbook.push_back(spell);
	}

	std::shared_ptr<Core::Spell> Player::getSpell(const int index)
	{
		if (index >= 0 && index < _spellbook.size()) return _spellbook[index];
		return nullptr;
	}

	void Player::updateSpells(const float dt) const
	{
		for (auto& s : _spellbook) s->update(dt);
	}

} // namespace Nawia::Entity