#pragma once
#include "Camera.h"
#include "Constants.h"
#include "EntityManager.h"
#include "Map.h"
#include "ResourceManager.h"

#include <Player.h>

#include <raylib.h>

namespace Nawia::Core {

	class PlayerController;

	class Engine {
	public:
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

		ResourceManager _resource_manager;
		Camera _camera;
		std::unique_ptr<Map> _map;
		std::unique_ptr<EntityManager> _entity_manager;
		std::shared_ptr<Entity::Player> _player;
		std::unique_ptr<PlayerController> _controller;
	};

} // namespace Nawia::Core