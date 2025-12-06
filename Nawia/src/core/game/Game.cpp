#include "Game.h"

#include <iostream>

namespace Nawia::Core {

    Game::Game() : _is_running(false), _window(nullptr) {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << SDL_GetError() << std::endl;
            return;
        }

        _window = SDL_CreateWindow(
            "Nawia",
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            SDL_WINDOW_RESIZABLE
        );

        if (!_window) {
            std::cerr << "Error while creating window" << SDL_GetError() << std::endl;
            SDL_Quit();
            return;
        }

        _is_running = true;
    }

    Game::~Game() {
        if (_window) {
            SDL_DestroyWindow(_window);
            _window = nullptr;
        }
        SDL_Quit();
    }

    bool Game::isRunning() const { return _is_running; }

    void Game::run() {
        while (isRunning()) {
            handleEvents();
            update();
            render();
        }
    }

    void Game::handleEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                _is_running = false;
            }
        }
    }

    void Game::update() {}

    void Game::render() {}

}; // namespace Nawia::Core