#pragma once

#include <SDL3/SDL.h>

namespace Nawia::Core {

constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;

class Game {
public:
    Game();
    ~Game();

    void run();
    bool isRunning() const;

private:
    void handleEvents();
    void update();
    void render();

    bool _is_running;
    SDL_Window* _window;
};

} // namespace Nawia::Core