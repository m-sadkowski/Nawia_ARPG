#include "Projectile.h"

namespace Nawia::Entity
{

	Projectile::Projectile(const float x, const float y, const float target_x, const float target_y, const float speed, const std::shared_ptr<SDL_Texture>& tex, const int damage, const float duration) 
		: SpellEffect(x, y, tex, duration, damage), _speed(speed)
	{
	    // calculate direction vector
		const float dx = target_x - x;
		const float dy = target_y - y;
	    const float length = std::sqrt(dx * dx + dy * dy);
	    _vel_x = (dx / length) * speed;
	    _vel_y = (dy / length) * speed;
	}

	void Projectile::update(const float dt) {
	    SpellEffect::update(dt);

	    _pos->setX(_pos->getX() + _vel_x * dt);
	    _pos->setY(_pos->getY() + _vel_y * dt);
	}

}

