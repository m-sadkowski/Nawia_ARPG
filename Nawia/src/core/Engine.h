#pragma once
#include "ResourceManager.h"
#include "Map.h"
#include "Camera.h"
#include "Constants.h"

#include <Player.h>

#include <SDL3/SDL.h>

namespace Nawia::Core {

    class PlayerController;

    class Engine {
    public:
        Engine();
        ~Engine();

        void run();
        [[nodiscard]] bool isRunning() const;
        void spawnEntity(std::unique_ptr<Entity::Entity> new_entity);

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

        std::shared_ptr<Entity::Player> _player;
        std::unique_ptr<PlayerController> _controller;

        uint64_t _last_time;

        Camera _camera;

        std::vector<std::unique_ptr<Entity::Entity>> _active_entities;
    };

} // namespace Nawia::Core