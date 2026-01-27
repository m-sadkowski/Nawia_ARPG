#include "DialogueUI.h"

namespace Nawia::UI {

    void DialogueUI::open(const Game::DialogueTree& tree) {
        _currentTree = tree;
        _currentNodeID = 0;
        _isOpen = true;
    }

    void DialogueUI::close() {
        _isOpen = false;
    }

    void DialogueUI::render() {
        if (!_isOpen) return;

        const auto* node = _currentTree.getNode(_currentNodeID);
        if (!node) { close(); return; }

        float screenW = (float)GetScreenWidth();
        float screenH = (float)GetScreenHeight();

        Rectangle panelRect = { 0, screenH - PANEL_HEIGHT, screenW, PANEL_HEIGHT };
        DrawRectangleRec(panelRect, Fade(BLACK, 0.8f));
        DrawRectangleLinesEx(panelRect, 2, WHITE);

        DrawText(node->speakerName.c_str(), 20, (int)panelRect.y + 10, 20, YELLOW);

        DrawText(node->text.c_str(), 20, (int)panelRect.y + 40, 20, WHITE);

        float optionY = panelRect.y + 80;
        for (const auto& option : node->options) {
            bool isHovered = false;
            DrawText(("- " + option.text).c_str(), 40, (int)optionY, 20, LIGHTGRAY);

            optionY += 30;
        }
    }

    bool DialogueUI::handleInput() {
        if (!_isOpen) return false;

        const auto* node = _currentTree.getNode(_currentNodeID);
        if (!node) return false;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse = GetMousePosition();
            float screenH = (float)GetScreenHeight();
            float startY = screenH - PANEL_HEIGHT + 80;

            for (const auto& option : node->options) {
                Rectangle optionRect = { 40, startY, 500, 25 };

                if (CheckCollisionPointRec(mouse, optionRect)) {

                    if (option.action != nullptr) {
                        option.action();
                    }

                    if (option.nextNodeID == -1) {
                        close();
                    }
                    else {
                        _currentNodeID = option.nextNodeID;
                    }
                    return true;
                }
                startY += 30;
            }
        }
        return true;
    }
}