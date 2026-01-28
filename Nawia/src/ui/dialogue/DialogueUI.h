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

        void render(const Font& font);
        bool handleInput();

        bool isOpen() const { return _is_open; }

    private:
        bool _is_open = false;
        Game::DialogueTree _current_tree;
        int _current_node_id = 0;

        const float PANEL_HEIGHT = 200.0f;
    };
}