#pragma once
#include "Camera.h"
#include "Constants.h"
#include "EntityManager.h"
#include "Map.h"
#include "ResourceManager.h"

#include <Player.h>
#include <UIHandler.h>

#include <raylib.h>

namespace Nawia::Core {

	class PlayerController;

	class Engine {
	public:
		enum class GameState {
			Menu,
			Playing
		};

		Engine();
		~Engine();

		void run();
		[[nodiscard]] bool isRunning() const;

		[[nodiscard]] std::shared_ptr<Entity::Entity> getEntityAt(float screen_x, float screen_y) const;
		void spawnEntity(const std::shared_ptr<Entity::Entity>& new_entity) const;

	private:
		void update(float delta_time);
		void render() const;
		void handleInput();

		bool _is_running;
		GameState _game_state;

		ResourceManager _resource_manager;
		Camera _camera;
		std::unique_ptr<Map> _map;
		std::unique_ptr<EntityManager> _entity_manager;
		std::shared_ptr<Entity::Player> _player;
		std::unique_ptr<PlayerController> _controller;
		std::unique_ptr<Nawia::UI::UIHandler> _ui_handler;
	};

} // namespace Nawia::Core