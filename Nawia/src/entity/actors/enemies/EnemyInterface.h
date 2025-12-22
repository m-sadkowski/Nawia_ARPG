#pragma once
#include "Entity.h"
#include "AnimationComponent.h"

#include <memory>

namespace Nawia::Core {
	class Map;
}

namespace Nawia::Entity {

	class EnemyInterface : public Entity {
	public:
		EnemyInterface(float x, float y, const std::shared_ptr<Texture2D>& tex, int max_hp, Core::Map* map);

		void update(float dt) override;
		void render(float offset_x, float offset_y) override;
		[[nodiscard]] bool isMouseOver(float mouse_x, float mouse_y, float cam_x, float cam_y) const override;

	protected:
		bool _is_moving;
		Core::Map* _map;

		std::unique_ptr<AnimationComponent> _animation_component;
	};

} // namespace Nawia::Entity