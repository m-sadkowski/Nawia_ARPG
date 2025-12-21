#include <Engine.h>

using namespace Nawia;

int main(int argc, char* argv[]) {
    std::cout << "Current path: " << std::filesystem::current_path() << std::endl;
    Core::Engine game;

    if (game.isRunning()) {
        game.run();
    }

    return 0;
}