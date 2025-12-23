#include "Engine.h"
#include "Logger.h"
#include "MathUtils.h"
#include "PlayerController.h"

#include <Dummy.h>
#include <FireballAbility.h>
#include <Projectile.h>
#include <SwordSlashAbility.h>

#include <algorithm>
#include <iostream>

namespace Nawia::Core {

	Engine::Engine() : _is_running(false), _controller(nullptr) 
	{
		SetTraceLogLevel(LOG_ERROR);
		InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Nawia");
		SetTargetFPS(60);

		// TEMPORARY SOLUTION
		// initialize map object
		_map = std::make_unique<Map>(_resource_manager);
		_map->loadMap("map1.json");

		// initialize player
		// TODO: Load Model here eventually, for now maybe texture
		auto player_texture = _resource_manager.getTexture("../assets/textures/player.png");
		Point2D player_spawn_pos = _map->getPlayerSpawnPos();
		_player = std::make_shared<Entity::Player>(player_spawn_pos.getX(), player_spawn_pos.getY(), player_texture);

		// initialize spells
		auto sword_slash_tex = _resource_manager.getTexture("../assets/textures/sword_slash.png");
		_player->addAbility(std::make_shared<Entity::SwordSlashAbility>(sword_slash_tex));
		auto fireball_tex = _resource_manager.getTexture("../assets/textures/fireball.png");
		_player->addAbility(std::make_shared<Entity::FireballAbility>(fireball_tex));

		// initialize player controller
		_controller = std::make_unique<PlayerController>(this, _player);

		// initialize entity manager
		_entity_manager = std::make_unique<EntityManager>();

		// spawn test enemy
		auto enemy_tex = _resource_manager.getTexture("../assets/textures/enemy.png");
		_entity_manager->addEntity(std::make_unique<Entity::Dummy>(15.0f, 15.0f, enemy_tex, 100, _map.get()));

		_is_running = true;
	}

	Engine::~Engine()
	{
		CloseWindow();
	}

	bool Engine::isRunning() const
	{
		return _is_running && !WindowShouldClose();
	}

	std::shared_ptr<Entity::Entity> Engine::getEntityAt(const float screen_x, const float screen_y) const 
	{
		return _entity_manager->getEntityAt(screen_x, screen_y, _camera);
	}

	void Engine::spawnEntity(const std::shared_ptr<Entity::Entity> &new_entity) const 
	{
		_entity_manager->addEntity(new_entity);
	}

	void Engine::run() 
	{
		while (isRunning()) 
		{
			const float delta_time = GetFrameTime();
			handleInput();
			update(delta_time);
			render();
		}
	}

	void Engine::handleInput() 
	{
		// transform mouse location to position in world
		Vector2 mouse_pos = GetMousePosition();
		Point2D mouse_world_pos =  Point2D::screenToIso(mouse_pos.x, mouse_pos.y, _camera.x, _camera.y);

		if (!_controller)
			return;

		_controller->handleInput(mouse_world_pos.getX(), mouse_world_pos.getY(), mouse_pos.x, mouse_pos.y);
	}

	void Engine::update(const float delta_time) 
	{
		if (!_player || !_entity_manager)
			return;

		_player->update(delta_time);
		_player->updateAbilities(delta_time);
		_camera.follow(_player.get());
		_controller->update(delta_time);

		_entity_manager->handleEntitiesCollisions();
		_entity_manager->updateEntities(delta_time);
	}

	void Engine::render() const 
	{
		BeginDrawing();
		ClearBackground(BLACK);

		if (!_map || !_player || !_entity_manager) 
		{
			EndDrawing();
			return;
		}

		/* RENDER START */

		// BeginMode2D(_camera); // If we use Raylib Camera2D, otherwise manual offset

		_map->render(_camera.x, _camera.y);
		_entity_manager->renderEntities(_camera);
		_player->render(_camera.x, _camera.y);

		/* RENDER END */

		EndDrawing();
	}

} // namespace Nawia::Core