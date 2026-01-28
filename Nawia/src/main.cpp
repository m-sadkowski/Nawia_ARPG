#include <Engine.h>

using namespace Nawia;

int main(int argc, char* argv[]) {
    Core::Engine game;

    if (game.isRunning()) {
        game.run();
    }

    return 0;
}
