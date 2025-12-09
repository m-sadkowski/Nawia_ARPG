#include "Engine.h"

#include <iostream>

namespace Nawia::Core {

    Engine::Engine() : _is_running(false), _window(nullptr) {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << SDL_GetError() << std::endl;
            return;
        }

        if (SDL_CreateWindowAndRenderer(
            "Nawia",
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            SDL_WINDOW_RESIZABLE,
            &_window,
            &_renderer
        ) < 0) {
            std::cerr << "Error while creating window" << SDL_GetError() << std::endl;
            SDL_Quit();
            return;
        }

        _map = std::make_unique<Map>(_renderer, _resourceManager);

        _map->loadTestMap();

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
            handleEvents();
            update();
            render();
        }
    }

    void Engine::handleEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                _is_running = false;
            }
        }
    }

    void Engine::update() {}

    void Engine::render() {
        SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
        SDL_RenderClear(_renderer);

        if (_map) {
            _map->render();
        }

        SDL_RenderPresent(_renderer);
    }

}; // namespace Nawia::Core