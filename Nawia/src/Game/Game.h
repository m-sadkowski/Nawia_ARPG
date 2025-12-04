#pragma once
#include <SDL3/SDL.h>

#include <functional>
#include <memory>

class Game {
public:
  Game();
  ~Game();

  void run();

private:
  void init();
  void handleEvents();
  void update();
  void render();
  void clean();

  using WindowPtr = std::unique_ptr<SDL_Window, void (*)(SDL_Window *)>;
  using RendererPtr = std::unique_ptr<SDL_Renderer, void (*)(SDL_Renderer *)>;

  WindowPtr window{nullptr, SDL_DestroyWindow};
  RendererPtr renderer{nullptr, SDL_DestroyRenderer};
  bool _isRunning = false;
  const int SCREEN_WIDTH = 800;
  const int SCREEN_HEIGHT = 600;
};
