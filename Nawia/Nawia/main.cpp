#include <SDL3/SDL.h>
#include <iostream>

constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;

class Game {
public:
    Game();
    ~Game();

    void run();
    bool isRunning() const { return running; }

private:
    void handleEvents();
    void update();
    void render();

    bool running;
    SDL_Window* window;
};

Game::Game() : running(false), window(nullptr) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << SDL_GetError() << std::endl;
        return;
    }

    window = SDL_CreateWindow(
        "TEST",
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_RESIZABLE
    );

    if (window == nullptr) {
        std::cerr << SDL_GetError() << std::endl;
        SDL_Quit();
        return;
    }

    running = true;
}

Game::~Game() {
    if (window != nullptr) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
}

void Game::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            running = false;
        }
    }
}

void Game::update() {}

void Game::render() {}

void Game::run() {
    while (isRunning()) {
        handleEvents();
        update();
        render();
        SDL_Delay(10);
    }
}

int main(int argc, char* argv[]) {
    Game game;

    if (game.isRunning()) {
        game.run();
    }

    return 0;
}