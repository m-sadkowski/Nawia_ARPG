#include "Engine.h"
#include "MathUtils.h"

#include <iostream>

#include "Logger.h"

namespace Nawia::Core {

    Engine::Engine() : _is_running(false), _window(nullptr), _renderer(nullptr) {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            std::cerr << SDL_GetError() << "\n";
            return;
        }

        if (!SDL_CreateWindowAndRenderer("Nawia", WINDOW_WIDTH, WINDOW_HEIGHT,SDL_WINDOW_RESIZABLE, &_window, &_renderer)) {
            Logger::errorLog("Error while creating window.");
            Logger::errorLog("SDL: " + std::string(SDL_GetError()));
            SDL_Quit();
            return;
        }

        // initialize map object
        _map = std::make_unique<Map>(_renderer, _resource_manager);

        // REMOVE - load test map
        _map->loadMap("map1.json");

        // player
        auto player_texture = _resource_manager.getTexture("../assets/textures/player.png", _renderer);
        _player = std::make_unique<Entity::Player>(10.0f, 10.0f, player_texture);

        // clock
        _last_time = SDL_GetTicks();
        _is_running = true;
    }

    Engine::~Engine() {
        if (_renderer) {
            SDL_DestroyRenderer(_renderer);
        }
        if (_window) {
            SDL_DestroyWindow(_window);
        }
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

    void Engine::handleEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                _is_running = false;
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    handleMouseClick(event.button.x, event.button.y);
                }
            }
        }
    }

    void Engine::update(const float delta_time) {
        if (_player) {
            _player->update(delta_time);

            _camera.follow(_player.get());
        }
    }

    void Engine::render() {
        SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255); // base background color
        SDL_RenderClear(_renderer);

        /*
            RENDER START
        */

        if (_map) {
            _map->render(_camera._x, _camera._y);
        }

        if (_player) {
            _player->render(_renderer, _camera._x, _camera._y);
        }

        /*
            RENDER END
        */

        SDL_RenderPresent(_renderer);
    }

    void Engine::handleMouseClick(const float mouse_x, const float mouse_y) {
        if (!_map || !_player)
            return;

        Point2D pos = Point2D::screenToIso(mouse_x, mouse_y, _camera._x, _camera._y);
        Logger::debugLog("Mouse raw: " + std::to_string(mouse_x) + ", " + std::to_string(mouse_y));
        Logger::debugLog("Iso float: " + std::to_string(pos.getX()) + ", " + std::to_string(pos.getY()));

        int target_x = static_cast<int>(std::floor(pos.getX()));
        int target_y = static_cast<int>(std::floor(pos.getY()));

        if (_map->isWalkable(target_x, target_y))
        {
            _player->moveTo(pos.getX(), pos.getY());
        }
    }

}; // namespace Nawia::Core