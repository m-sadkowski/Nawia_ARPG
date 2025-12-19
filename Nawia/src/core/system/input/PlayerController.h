#pragma once
#include <Player.h>

#include <SDL3/SDL.h>
#include <memory>

namespace Nawia::Core 
{

	class Engine;

	class PlayerController {
	public:
	  PlayerController(Engine *engine, std::shared_ptr<Entity::Player> player);

	  void handleInput(const SDL_Event &event, float mouse_world_x, float mouse_world_y, float screen_x, float screen_y) const;

	private:
	  Engine* _engine;
	  std::shared_ptr<Entity::Player> _player;

	  void useAbility(int index, float target_x, float target_y) const;
	};

} // namespace Nawia::Core