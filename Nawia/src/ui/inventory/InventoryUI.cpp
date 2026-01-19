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

        const auto& backpackItems = player.getBackpack().getItems();

        Vector2 mousePos = GetMousePosition();

        for (int i = 0; i < COLS * ROWS; ++i) {
            int col = i % COLS;
            int row = i / COLS;

            float slotX = _position.x + PADDING + (col * (SLOT_SIZE + PADDING));
            float slotY = _position.y + 35 + (row * (SLOT_SIZE + PADDING));

            // check for mouse
            Rectangle slotRect = { slotX, slotY, SLOT_SIZE, SLOT_SIZE };
            bool isHovered = CheckCollisionPointRec(mousePos, slotRect);

            std::shared_ptr<Item::Item> itemToDraw = nullptr;
            if (i < backpackItems.size()) {
                itemToDraw = backpackItems[i];
            }

            drawSlot(i, slotX, slotY, isHovered, itemToDraw);
        }
    }

    void InventoryUI::drawSlot(int index, float x, float y, bool isHovered, const std::shared_ptr<Item::Item>& item) const {
        // slot background
        Color slotColor = isHovered ? LIGHTGRAY : DARKGRAY;
        DrawRectangle(x, y, SLOT_SIZE, SLOT_SIZE, slotColor);
        DrawRectangleLines(x, y, SLOT_SIZE, SLOT_SIZE, WHITE);

        // TODO: draw item
        if (item != nullptr) {
            Texture2D icon = item->getIcon();

            // check if texture is loaded
            if (icon.id > 0) {
                float padding = 4.0f;
                Rectangle source = { 0.0f, 0.0f, (float)icon.width, (float)icon.height };

                Rectangle dest = {
                    x + padding,
                    y + padding,
                    SLOT_SIZE - (padding * 2),
                    SLOT_SIZE - (padding * 2)
                };

                DrawTexturePro(icon, source, dest, { 0, 0 }, 0.0f, WHITE);
            }

            if (isHovered) {
                const char* text = item->getName().c_str();
                int textWidth = MeasureText(text, 10);

                DrawRectangle(x, y - 15, textWidth + 4, 12, BLACK);
                DrawText(text, x + 2, y - 14, 10, YELLOW);
            }
        }
    }

    int InventoryUI::handleInput() const {
        // TODO handle input logic
        return -1;
    }

}