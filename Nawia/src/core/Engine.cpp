#include "Engine.h"
#include "MathUtils.h"
#include "Logger.h"
#include "Spell.h"
#include "PlayerController.h" 
#include "FireballSpell.h"

#include <iostream>
#include <algorithm>

namespace Nawia::Core {

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
        _player = std::make_shared<Entity::Player>(10.0f, 10.0f, player_texture);

        // TEMPORARY SOLUTION
        // initialize spells
        auto fireball_tex = _resource_manager.getTexture("../assets/textures/fireball.png", _renderer);
        _player->addSpell(std::make_shared<FireballSpell>(fireball_tex));

		// initialize player controller
        _controller = std::make_unique<PlayerController>(this, _player);

        // clock
        _last_time = SDL_GetTicks();
        _is_running = true;
    }

    Engine::~Engine() {
        if (_renderer)
            SDL_DestroyRenderer(_renderer);
        if (_window)
            SDL_DestroyWindow(_window);
        SDL_Quit();
    }

    bool Engine::isRunning() const { return _is_running; }

    void Engine::run() {
        while (isRunning()) {
            const uint64_t current_time = SDL_GetTicks();
            const float delta_time = static_cast<float>(current_time - _last_time) / 1000.0f;
            _last_time = current_time;

            handleEvents();
            update(delta_time);
            render();
        }
    }

    void Engine::spawnEntity(std::unique_ptr<Entity::Entity> new_entity) {
        _active_entities.push_back(std::move(new_entity));
    }

    void Engine::handleEvents() {
        SDL_Event event;

        // transform mouse location to position in world
        float mouse_screen_x, mouse_screen_y;
        SDL_GetMouseState(&mouse_screen_x, &mouse_screen_y);
        Point2D mouse_world_pos = Point2D::screenToIso(mouse_screen_x, mouse_screen_y, _camera._x, _camera._y);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                _is_running = false;
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    handleMouseClick(event.button.x, event.button.y);
                }
            }

            if (_controller) {
                _controller->handleInput(event, mouse_world_pos.getX(), mouse_world_pos.getY());
            }
        }
    }

    void Engine::update(const float delta_time) {
        if (_player) {
            _player->update(delta_time);
            _player->updateSpells(delta_time);
            _camera.follow(_player.get());
        }

        // handle collisions
        // TODO

        // update active entities
        for (auto it = _active_entities.begin(); it != _active_entities.end(); ) {
            auto& entity = *it;
            entity->update(delta_time);

        	auto spellEffect = dynamic_cast<Entity::SpellEffect*>(entity.get());
            bool expired = (spellEffect && spellEffect->isExpired());

            if (entity->isDead() || expired)
                it = _active_entities.erase(it);
            else
                ++it;
        }
    }

    void Engine::render() {
        SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
        SDL_RenderClear(_renderer);

        /* RENDER START */

        if (_map) {
            _map->render(_camera._x, _camera._y);
        }

        for (const auto& entity : _active_entities) {
            entity->render(_renderer, _camera._x, _camera._y);
        }

        if (_player) {
            _player->render(_renderer, _camera._x, _camera._y);
        }

        /* RENDER END */

        SDL_RenderPresent(_renderer);
    }

    void Engine::handleMouseClick(const float mouse_x, const float mouse_y) {
        if (!_map || !_player) return;

        Point2D pos = Point2D::screenToIso(mouse_x, mouse_y, _camera._x, _camera._y);
        int target_x = static_cast<int>(std::floor(pos.getX()));
        int target_y = static_cast<int>(std::floor(pos.getY()));

        if (_map->isWalkable(target_x, target_y)) {
            _player->moveTo(pos.getX(), pos.getY());
        }
    }

}; // namespace Nawia::Core