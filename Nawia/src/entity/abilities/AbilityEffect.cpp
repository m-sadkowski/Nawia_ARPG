#include "AbilityEffect.h"

namespace Nawia::Entity 
{

	AbilityEffect::AbilityEffect(const float x, const float y, const std::shared_ptr<SDL_Texture>& tex, const float duration, const int damage) : Entity(x, y, tex, 1), _duration(duration), _damage(damage), _timer(0.0f) {}

	void AbilityEffect::update(const float dt) {
		_timer += dt;
	}

	bool AbilityEffect::isExpired() const { return _timer >= _duration; }

	int AbilityEffect::getDamage() const { return _damage; }

} // namespace Nawia::Entity