#include "EnemyInterface.h"
#include "AnimationComponent.h"

#include <MathUtils.h>
#include <Constants.h>

namespace Nawia::Entity {

	EnemyInterface::EnemyInterface(const float x, const float y, const std::shared_ptr<Texture2D>& tex, const int max_hp, Core::Map* map)
		: Entity(x, y, tex, max_hp), _is_moving(false), _map(map) {
	}

	void EnemyInterface::update(const float dt)
	{
		if (_animation_component)
			_animation_component->update(dt);
	}

	void EnemyInterface::render(const float offset_x, const float offset_y)
	{
		Core::Point2D pos = getScreenPos(getX(), getY(), offset_x, offset_y);

		if (_animation_component)
		{
			const Vector2 screen_pos = { pos.getX(), pos.getY() };
			constexpr float model_scale = 3.0f;
			_animation_component->render(screen_pos, model_scale);
		}
		else
		{
			Entity::render(offset_x, offset_y);
		}
	}

	bool EnemyInterface::isMouseOver(const float mouse_x, const float mouse_y, const float cam_x, const float cam_y) const
	{
		Core::Point2D pos = getScreenPos(getX(), getY(), cam_x, cam_y);
		const float screen_x = pos.getX();
		const float screen_y = pos.getY();

		constexpr float width = Core::ENTITY_TEXTURE_WIDTH * 1.0f;
		constexpr float height = Core::ENTITY_TEXTURE_HEIGHT * 2.0f;

		const float left = screen_x - (width / 2.0f);
		const float right = screen_x + (width / 2.0f);
		const float top = screen_y - height;
		const float bottom = screen_y;

		return (mouse_x >= left && mouse_x <= right && mouse_y >= top && mouse_y <= bottom);
	}

} // namespace Nawia::Entity