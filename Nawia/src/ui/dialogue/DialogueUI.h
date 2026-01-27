#pragma once
#include <raylib.h>
#include "Dialogue.h"
#include "Cat.h"
#include <memory>

namespace Nawia::UI {

    class DialogueUI {
    public:
        void open(const Game::DialogueTree& tree);
        void close();

        void render();
        bool handleInput();

        bool isOpen() const { return _isOpen; }

    private:
        bool _isOpen = false;
        Game::DialogueTree _currentTree;
        int _currentNodeID = 0;

        const float PANEL_HEIGHT = 200.0f;
    };
}