#include "EnemyInterface.h"

#include <MathUtils.h>
#include <Constants.h>

namespace Nawia::Entity {

	EnemyInterface::EnemyInterface(const std::string& name, const float x, const float y, const std::shared_ptr<Texture2D>& texture, const int max_hp, Core::Map* map)
		: Entity(name, x, y, texture, max_hp), _is_moving(false), _map(map) {}

	bool EnemyInterface::isMouseOver(const float mouse_x, const float mouse_y, const float cam_x, const float cam_y) const
	{
		Vector2 pos = getScreenPos(getX(), getY(), cam_x, cam_y);
		const float screen_x = pos.x;
		const float screen_y = pos.y;

		constexpr float width = Core::ENTITY_TEXTURE_WIDTH * 1.0f;
		constexpr float height = Core::ENTITY_TEXTURE_HEIGHT * 2.0f;

		const float left = screen_x - (width / 2.0f);
		const float right = screen_x + (width / 2.0f);
		const float top = screen_y - height;
		const float bottom = screen_y;

		return (mouse_x >= left && mouse_x <= right && mouse_y >= top && mouse_y <= bottom);
	}

} // namespace Nawia::Entity