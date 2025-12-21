#include "Enemy.h"

namespace Nawia::Entity {

	Enemy::Enemy(const float x, const float y, const std::shared_ptr<SDL_Texture>& tex, const int max_hp, Core::Map* map)
		: Entity(x, y, tex, max_hp), _map(map), _is_moving(false) {}

} // namespace Nawia::Entity
