#include "Engine.h"

#include <iostream>
#include "MathUtils.h"

namespace Nawia::Core {

    Engine::Engine() : _is_running(false), _window(nullptr), _renderer(nullptr) {
        // init SDL
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            std::cerr << SDL_GetError() << std::endl;
            return;
        }

        // create window and renderer
        if (!SDL_CreateWindowAndRenderer(
            "Nawia",
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            SDL_WINDOW_RESIZABLE,
            &_window,
            &_renderer
        )) {
            std::cerr << "Error while creating window" << SDL_GetError() << std::endl;
            SDL_Quit();
            return;
        }

        // initialize map object
        _map = std::make_unique<Map>(_renderer, _resourceManager);

        // REMOVE - load test map
        _map->loadTestMap();

        // player
        auto playerTexture = _resourceManager.getTexture("../assets/textures/player.png", _renderer);
        
        _player = std::make_unique<Entity::Player>(5.0f, 5.0f, playerTexture);

        // clock
        _lastTime = SDL_GetTicks();

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
            uint64_t currentTime = SDL_GetTicks();
            float deltaTime = (currentTime - _lastTime) / 1000.0f;
            _lastTime = currentTime;

            handleEvents();
            update(deltaTime);
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

    void Engine::update(float deltaTime) {
        if (_player) {
            _player->update(deltaTime);
        }
    }

    void Engine::render() {
        // base background color
        SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
        SDL_RenderClear(_renderer);

        /*
            RENDER START
        */
        if (_map) {
            _map->render();
        }

        if (_player) {
            _player->render(_renderer, 500.0f, 0.0f);
        }

        /*
            RENDER END
        */

        SDL_RenderPresent(_renderer);
    }

    void Engine::handleMouseClick(float mouseX, float mouseY) {
        if (!_player) return;

        Point2D pos = Point2D::screenToIso(mouseX, mouseY);

        // DEBUG
        std::cout << "[DEBUG] Klik: " << pos.getX() << ", " << pos.getY() << std::endl;
        _player->moveTo(pos.getX(), pos.getY());
    }

}; // namespace Nawia::Core