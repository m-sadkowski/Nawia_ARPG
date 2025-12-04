#include "Game.h"
#include <iostream>

Game::Game() {}

Game::~Game() { clean(); }

void Game::init() {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError()
              << std::endl;
    return;
  }

  window.reset(SDL_CreateWindow("Nawia", SCREEN_WIDTH, SCREEN_HEIGHT, 0));
  if (!window) {
    std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError()
              << std::endl;
    return;
  }

  renderer.reset(SDL_CreateRenderer(window.get(), NULL));
  if (!renderer) {
    std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError()
              << std::endl;
    return;
  }

  _isRunning = true;
}

void Game::run() {
  init();

  while (_isRunning) {
    handleEvents();
    update();
    render();
  }

  clean();
}

void Game::handleEvents() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) {
      _isRunning = false;
    }
  }
}

void Game::update() {}

void Game::render() {
  SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255);
  SDL_RenderClear(renderer.get());
  SDL_RenderPresent(renderer.get());
}

void Game::clean() { SDL_Quit(); }
