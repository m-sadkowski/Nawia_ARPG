#pragma once

#include <SDL3/SDL.h>
#include <string>

#include "ResourceManager.h"
#include "Map.h"
#include "Player.h"

namespace Nawia::Core {

    constexpr int WINDOW_WIDTH = 1280;
    constexpr int WINDOW_HEIGHT = 720;

    class Engine {
    public:
        Engine();
        ~Engine();

        void run();
        bool isRunning() const;

    private:
        void handleEvents();
        void update(float deltaTime);
        void render();

        void handleMouseClick(float mouseX, float mouseY);

        bool _is_running;
        SDL_Window* _window;
        SDL_Renderer* _renderer;

        // Resource Manager - holds all loaded textures
        ResourceManager _resourceManager;

        std::unique_ptr<Map> _map;

        std::unique_ptr<Entity::Player> _player;

        uint64_t _lastTime;
    };

} // namespace Nawia::Core