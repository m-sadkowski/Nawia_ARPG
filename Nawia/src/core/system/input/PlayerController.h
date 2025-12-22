#pragma once
#include <Player.h>

#include <raylib.h>
#include <memory>

namespace Nawia::Core {

	class Engine;

	class PlayerController {
	public:
		PlayerController(Engine *engine, std::shared_ptr<Entity::Player> player);

		void handleInput(float mouse_world_x, float mouse_world_y, float screen_x, float screen_y);
		void update(float dt);

	private:
		Engine* _engine;
		std::shared_ptr<Entity::Player> _player;
		std::shared_ptr<Entity::Entity> _target_enemy;

		void useAbility(int index, float target_x, float target_y) const;
	};

} // namespace Nawia::Core