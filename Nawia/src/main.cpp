#include <Game.h>

using namespace Nawia;

int main(int argc, char* argv[]) {
    Core::Game game;

    if (game.isRunning()) {
        game.run();
    }

    return 0;
}