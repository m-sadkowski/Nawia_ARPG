#include "InventoryUI.h"
#include <string>

namespace Nawia::UI {

    InventoryUI::InventoryUI() {
        _position = { 200, 200 };
    }

    void InventoryUI::render(const Entity::Player& player) const {
        // background
        float bgWidth = (SLOT_SIZE + PADDING) * COLS + PADDING;
        float bgHeight = (SLOT_SIZE + PADDING) * ROWS + PADDING + 30;

        DrawRectangle(_position.x, _position.y, bgWidth, bgHeight, Fade(BLACK, 0.8f));
        DrawRectangleLines(_position.x, _position.y, bgWidth, bgHeight, WHITE);

        DrawText("INVENTORY", _position.x + PADDING, _position.y + 5, 20, WHITE);

        Vector2 mousePos = GetMousePosition();

        for (int i = 0; i < COLS * ROWS; ++i) {
            int col = i % COLS;
            int row = i / COLS;

            float slotX = _position.x + PADDING + (col * (SLOT_SIZE + PADDING));
            float slotY = _position.y + 35 + (row * (SLOT_SIZE + PADDING));

            // check for mouse
            Rectangle slotRect = { slotX, slotY, SLOT_SIZE, SLOT_SIZE };
            bool isHovered = CheckCollisionPointRec(mousePos, slotRect);

            drawSlot(i, slotX, slotY, isHovered);
        }
    }

    void InventoryUI::drawSlot(int index, float x, float y, bool isHovered) const {
        // slot background
        Color slotColor = isHovered ? LIGHTGRAY : DARKGRAY;
        DrawRectangle(x, y, SLOT_SIZE, SLOT_SIZE, slotColor);
        DrawRectangleLines(x, y, SLOT_SIZE, SLOT_SIZE, WHITE);

        // TODO: draw item

        // DEBUG slot number
        DrawText(std::to_string(index).c_str(), x + 2, y + 2, 10, WHITE);
    }

    int InventoryUI::handleInput() const {
        // TODO handle input logic
        return -1;
    }

}