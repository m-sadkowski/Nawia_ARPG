#include "Engine.h"
#include "Enemy.h"
#include "Logger.h"
#include "MathUtils.h"
#include "PlayerController.h"

#include <Projectile.h>
#include <FireballAbility.h>
#include <SwordSlashAbility.h>

#include <algorithm>
#include <iostream>

namespace Nawia::Core 
{

    Engine::Engine() : _is_running(false), _window(nullptr), _renderer(nullptr), _controller(nullptr) {
      if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << SDL_GetError() << "\n";
        return;
      }

      if (!SDL_CreateWindowAndRenderer("Nawia", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, &_window, &_renderer)) {
        Logger::errorLog("Error while creating window.");
        Logger::errorLog("SDL: " + std::string(SDL_GetError()));
        SDL_Quit();
        return;
      }

      // initialize map object
      _map = std::make_unique<Map>(_renderer, _resource_manager);
      _map->loadMap("map1.json");

      // initialize player
      auto player_texture = _resource_manager.getTexture("../assets/textures/player.png", _renderer);
      Point2D player_spawn_pos = _map->getPlayerSpawnPos();
      _player = std::make_shared<Entity::Player>(player_spawn_pos.getX(), player_spawn_pos.getY(), player_texture);

      // TEMPORARY SOLUTION
      // initialize spells
      auto sword_slash_tex = _resource_manager.getTexture("../assets/textures/sword_slash.png", _renderer);
      _player->addAbility(std::make_shared<Entity::SwordSlashAbility>(sword_slash_tex));
      auto fireball_tex = _resource_manager.getTexture("../assets/textures/fireball.png", _renderer);
      _player->addAbility(std::make_shared<Entity::FireballAbility>(fireball_tex));

      // initialize player controller
      _controller = std::make_unique<PlayerController>(this, _player);

      // initialize entity manager
      _entity_manager = std::make_unique<EntityManager>();

      // spawn test enemy
      auto enemy_tex = _resource_manager.getTexture("../assets/textures/enemy.png", _renderer);
      _entity_manager->addEntity(std::make_unique<Entity::Enemy>(15.0f, 15.0f, enemy_tex, 100));

      // clock
      _last_time = SDL_GetTicks();
      _is_running = true;
    }
	
	Engine::~Engine() 
    {
      if (_renderer)
        SDL_DestroyRenderer(_renderer);
      if (_window)
        SDL_DestroyWindow(_window);
      SDL_Quit();
    }

    bool Engine::isRunning() const { return _is_running; }

    std::shared_ptr<Entity::Entity> Engine::getEntityAt(const float screen_x, const float screen_y) const 
    {
      return _entity_manager->getEntityAt(screen_x, screen_y, _camera);
    }

    void Engine::spawnEntity(const std::shared_ptr<Entity::Entity>& new_entity) const 
    {
      _entity_manager->addEntity(new_entity);
    }

    void Engine::run() 
    {
      while (isRunning()) {
        const uint64_t current_time = SDL_GetTicks();
        const float delta_time =
            static_cast<float>(current_time - _last_time) / 1000.0f;
        _last_time = current_time;

        handleEvents();
        update(delta_time);
        render();
      }
    }

    void Engine::handleEvents() 
    {
      SDL_Event event;

      // transform mouse location to position in world
      float mouse_screen_x, mouse_screen_y;
      SDL_GetMouseState(&mouse_screen_x, &mouse_screen_y);
      Point2D mouse_world_pos = Point2D::screenToIso(mouse_screen_x, mouse_screen_y, _camera.x, _camera.y);

      while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
          _is_running = false;
        } 
  	    else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
          if (event.button.button == SDL_BUTTON_LEFT)
            handleMouseClick(event.button.x, event.button.y);
        }

        if (_controller)
          _controller->handleInput(event, mouse_world_pos.getX(),mouse_world_pos.getY(), mouse_screen_x,mouse_screen_y);
      }
    }

    void Engine::update(const float delta_time) {
      if (!_player || !_entity_manager)
        return;

      _player->update(delta_time);
      _player->updateAbilities(delta_time);
      _camera.follow(_player.get());

      _entity_manager->handleEntitiesCollisions();
      _entity_manager->updateEntities(delta_time);
    }

    void Engine::render() const {
      SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
      SDL_RenderClear(_renderer);

      if (!_map || !_player || !_entity_manager)
        return;

      /* RENDER START */

      _map->render(_camera.x, _camera.y);
      _entity_manager->renderEntities(_renderer, _camera);
      _player->render(_renderer, _camera.x, _camera.y);

      /* RENDER END */

      SDL_RenderPresent(_renderer);
    }

    void Engine::handleMouseClick(const float mouse_x, const float mouse_y) const {
      if (!_map || !_player)
        return;

      Point2D pos = Point2D::screenToIso(mouse_x, mouse_y, _camera.x, _camera.y);
      int target_x = static_cast<int>(std::floor(pos.getX()));
      int target_y = static_cast<int>(std::floor(pos.getY()));

      if (_map->isWalkable(target_x, target_y)) {
        _player->moveTo(pos.getX(), pos.getY());
      }
    }

}; // namespace Nawia::Core