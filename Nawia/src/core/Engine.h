#pragma once
#include "ResourceManager.h"
#include "Map.h"

#include <Player.h>

#include <SDL3/SDL.h>

namespace Nawia::Core {

    constexpr int WINDOW_WIDTH = 1280;
    constexpr int WINDOW_HEIGHT = 720;

    class Engine {
    public:
        Engine();
        ~Engine();

        void run();
        [[nodiscard]] bool isRunning() const;

    private:
        void handleEvents();
        void update(float delta_time);
        void render();

        void handleMouseClick(float mouse_x, float mouse_y);

        bool _is_running;
        SDL_Window* _window;
        SDL_Renderer* _renderer;

        // Resource Manager - holds all loaded textures
        ResourceManager _resource_manager;
        std::unique_ptr<Map> _map;
        std::unique_ptr<Entity::Player> _player;
        uint64_t _last_time;
    };

} // namespace Nawia::Core