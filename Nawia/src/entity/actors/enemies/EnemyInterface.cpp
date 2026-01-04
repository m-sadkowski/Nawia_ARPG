#include "EnemyInterface.h"

#include <MathUtils.h>
#include <Constants.h>

namespace Nawia::Entity {

	EnemyInterface::EnemyInterface(const std::string& name, const float x, const float y, 
									const std::shared_ptr<Texture2D>& texture, const int max_hp, Core::Map* map)
		: Entity(name, x, y, texture, max_hp), _is_moving(false), _map(map) {}


} // namespace Nawia::Entity