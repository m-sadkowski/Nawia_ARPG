#pragma once

#include <Dialogue.h>

#include <raylib.h>

#include <memory>
#include <vector>

namespace Nawia::UI {

    class DialogueUI {
    public:
        /** @brief Otwiera drzewo dialogowe od pierwszego wezla. */
        void open(const Game::DialogueTree& tree);

        /** @brief Zamyka aktualny dialog. */
        void close();

        /** @brief Rysuje aktualny wezel dialogowy z dynamicznie dobrana wysokoscia. */
        void render(const Font& font);

        /** @brief Obsluguje klikniecia w opcje dialogowe. */
        bool handleInput();

        /** @brief Zwraca, czy dialog jest aktualnie otwarty. */
        [[nodiscard]] bool isOpen() const { return _is_open; }

    private:
        bool _is_open = false;
        Game::DialogueTree _current_tree;
        int _current_node_id = 0;
        std::vector<Rectangle> _option_rectangles;
    };
} // namespace Nawia::UI
