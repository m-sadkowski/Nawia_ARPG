#include "InventoryUI.h"
#include <string>

namespace Nawia::UI {

    InventoryUI::InventoryUI() {
        _position = { 200, 200 };
    }

    void InventoryUI::render(const Entity::Player& player) const {
        float startX = 100.0f;
        float startY = 100.0f;
        float width = 500.0f;
        float height = 400.0f;

        // background
        DrawRectangle(startX, startY, width, height, Fade(BLACK, 0.9f));
        DrawRectangleLines(startX, startY, width, height, WHITE);

        DrawLine(startX + 220, startY, startX + 220, startY + height, WHITE);

        DrawText("EQUIPMENT", startX + 60, startY + 10, 20, WHITE);
        DrawText("BACKPACK", startX + 240, startY + 10, 20, WHITE);

        Vector2 mousePos = GetMousePosition();

        float eqCenterX = startX + 110;
        float eqTopY = startY + 50;

        drawSpecificSlot(Item::EquipmentSlot::Head, eqCenterX - 25, eqTopY, player, mousePos);
        drawSpecificSlot(Item::EquipmentSlot::Neck, eqCenterX + 40, eqTopY, player, mousePos);
        drawSpecificSlot(Item::EquipmentSlot::Chest, eqCenterX - 25, eqTopY + 60, player, mousePos);
        drawSpecificSlot(Item::EquipmentSlot::Legs, eqCenterX - 25, eqTopY + 120, player, mousePos);
        drawSpecificSlot(Item::EquipmentSlot::Feet, eqCenterX - 25, eqTopY + 180, player, mousePos);
        drawSpecificSlot(Item::EquipmentSlot::Weapon, eqCenterX - 90, eqTopY + 60, player, mousePos);
        drawSpecificSlot(Item::EquipmentSlot::OffHand, eqCenterX + 40, eqTopY + 60, player, mousePos);
        drawSpecificSlot(Item::EquipmentSlot::Ring, eqCenterX - 90, eqTopY + 120, player, mousePos);


        const auto& backpackItems = player.getBackpack().getItems();
        float backpackX = startX + 240;
        float backpackY = startY + 50;

        for (int i = 0; i < 20; ++i) { // 20 slotów
            int col = i % 4; // 4 kolumny
            int row = i / 4; // 5 rzędów

            float slotX = backpackX + (col * (SLOT_SIZE + 10));
            float slotY = backpackY + (row * (SLOT_SIZE + 10));

            bool isHovered = CheckCollisionPointRec(mousePos, { slotX, slotY, SLOT_SIZE, SLOT_SIZE });

            std::shared_ptr<Item::Item> item = (i < backpackItems.size()) ? backpackItems[i] : nullptr;

            drawSlot(i, slotX, slotY, isHovered, item);
        }

        std::string goldText = "Gold: " + std::to_string(player.getGold());
        DrawText(goldText.c_str(), startX + 240, startY + height - 30, 20, GOLD);
    }

    void InventoryUI::drawSpecificSlot(Item::EquipmentSlot slotType, float x, float y, const Entity::Player& player, Vector2 mousePos) const {
        auto item = player.getEquipment().getItemAt(slotType);

        bool isHovered = CheckCollisionPointRec(mousePos, { x, y, SLOT_SIZE, SLOT_SIZE });

        drawSlot(-1, x, y, isHovered, item);

        if (item == nullptr) {
            DrawText("?", x + 15, y + 15, 20, Fade(WHITE, 0.2f));
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
        if (!IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            return -1;
        }

        Vector2 mousePos = GetMousePosition();

        // Obliczamy pozycję plecaka (taka sama matematyka jak w render)
        float startX = 100.0f;
        float startY = 100.0f;
        float backpackX = startX + 240;
        float backpackY = startY + 50;

        for (int i = 0; i < 20; ++i) {
            int col = i % 4;
            int row = i / 4;
            float slotX = backpackX + (col * (SLOT_SIZE + 10));
            float slotY = backpackY + (row * (SLOT_SIZE + 10));

            Rectangle slotRect = { slotX, slotY, SLOT_SIZE, SLOT_SIZE };

            if (CheckCollisionPointRec(mousePos, slotRect)) {
                return i;
            }
        }

        return -1;
    }

    Item::EquipmentSlot InventoryUI::getClickedEquipmentSlot() const {
        if (!IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            return Item::EquipmentSlot::None;
        }

        Vector2 mousePos = GetMousePosition();

        float startX = 100.0f;
        float startY = 100.0f;
        float cx = startX + 110;
        float cy = startY + 50;
        float size = SLOT_SIZE;

        struct SlotHitbox {
            Item::EquipmentSlot slot;
            float x;
            float y;
        };

        SlotHitbox hitboxes[] = {
            { Item::EquipmentSlot::Head,    cx - 25, cy },
            { Item::EquipmentSlot::Neck,    cx + 40, cy },
            { Item::EquipmentSlot::Chest,   cx - 25, cy + 60 },
            { Item::EquipmentSlot::Legs,    cx - 25, cy + 120 },
            { Item::EquipmentSlot::Feet,    cx - 25, cy + 180 },
            { Item::EquipmentSlot::Weapon,  cx - 90, cy + 60 },
            { Item::EquipmentSlot::OffHand, cx + 40, cy + 60 },
            { Item::EquipmentSlot::Ring,    cx - 90, cy + 120 }
        };

        for (const auto& hb : hitboxes) {
            Rectangle rect = { hb.x, hb.y, size, size };

            if (CheckCollisionPointRec(mousePos, rect)) {
                return hb.slot;
            }
        }

        return Item::EquipmentSlot::None;
    }

}