#include "InventoryUI.h"
#include "ResourceManager.h"
#include <string>

namespace Nawia::UI {

    InventoryUI::InventoryUI() {
        _position = { 200, 200 };
    }

    void InventoryUI::loadResources(Core::ResourceManager& resourceManager) {
        _placeholders[Item::EquipmentSlot::Head] = resourceManager.getTexture("../assets/textures/ui/slot_head.png");
    }

    void InventoryUI::render(const Font& font, const Entity::Player& player) const {
        // background
        DrawRectangle(INV_START_X, INV_START_Y, INV_WIDTH, INV_HEIGHT, Fade(BLACK, 0.9f));
        DrawRectangleLines(INV_START_X, INV_START_Y, INV_WIDTH, INV_HEIGHT, WHITE);

        DrawLine(INV_START_X + EQ_WIDTH, INV_START_Y, INV_START_X + EQ_WIDTH, INV_START_Y + INV_HEIGHT, WHITE);

        DrawTextEx(font, "EQUIPMENT", { INV_START_X + TEXT_PADDING_LEFT, INV_START_Y + TEXT_PADDING_TOP }, 20, 1.0f, WHITE);
        DrawTextEx(font, "BACKPACK", { INV_START_X + EQ_WIDTH + TEXT_PADDING_LEFT, INV_START_Y + TEXT_PADDING_TOP }, 20, 1.0f, WHITE);

        Vector2 mousePos = GetMousePosition();

        float eqCenterX = INV_START_X + EQ_WIDTH/2;
        float eqTopY = INV_START_Y + EQ_START_TOP;

        drawSpecificSlot(Item::EquipmentSlot::Head, eqCenterX - 25, eqTopY, player, mousePos);
        drawSpecificSlot(Item::EquipmentSlot::Neck, eqCenterX + 40, eqTopY, player, mousePos);
        drawSpecificSlot(Item::EquipmentSlot::Chest, eqCenterX - 25, eqTopY + 60, player, mousePos);
        drawSpecificSlot(Item::EquipmentSlot::Legs, eqCenterX - 25, eqTopY + 120, player, mousePos);
        drawSpecificSlot(Item::EquipmentSlot::Feet, eqCenterX - 25, eqTopY + 180, player, mousePos);
        drawSpecificSlot(Item::EquipmentSlot::Weapon, eqCenterX - 90, eqTopY + 60, player, mousePos);
        drawSpecificSlot(Item::EquipmentSlot::OffHand, eqCenterX + 40, eqTopY + 60, player, mousePos);
        drawSpecificSlot(Item::EquipmentSlot::Ring, eqCenterX - 90, eqTopY + 120, player, mousePos);


        const auto& backpackItems = player.getBackpack().getItems();
        float backpackX = INV_START_X + EQ_WIDTH + TEXT_PADDING_LEFT;
        float backpackY = INV_START_Y + BP_START_TOP;

        for (int i = 0; i < 20; ++i) {
            int col = i % 4;
            int row = i / 4;

            float slotX = backpackX + (col * (SLOT_SIZE + 10));
            float slotY = backpackY + (row * (SLOT_SIZE + 10));

            bool isHovered = CheckCollisionPointRec(mousePos, { slotX, slotY, SLOT_SIZE, SLOT_SIZE });

            std::shared_ptr<Item::Item> item = (i < backpackItems.size()) ? backpackItems[i] : nullptr;

            drawSlot(i, slotX, slotY, isHovered, item);
        }

        std::string goldText = "GOLD: " + std::to_string(player.getGold());
        DrawTextEx(font, goldText.c_str(), { INV_START_X + EQ_WIDTH + TEXT_PADDING_LEFT, INV_START_Y + INV_HEIGHT - GOLD_PADDING_BOTTOM }, 20, 1.0f, GOLD);
    }

    void InventoryUI::drawSpecificSlot(Item::EquipmentSlot slotType, float x, float y, const Entity::Player& player, Vector2 mousePos) const {
        auto item = player.getEquipment().getItemAt(slotType);

        bool isHovered = CheckCollisionPointRec(mousePos, { x, y, SLOT_SIZE, SLOT_SIZE });

        drawSlot(-1, x, y, isHovered, item);

        if (item == nullptr) {
            if (_placeholders.count(slotType)) {
                auto texPtr = _placeholders.at(slotType);

                if (texPtr && texPtr->id > 0) {
                    Texture2D tex = *texPtr;

                    Rectangle source = { 0, 0, (float)tex.width, (float)tex.height };
                    Rectangle dest = {
                        x + SLOT_PLACEHOLDER_PADDING,
                        y + SLOT_PLACEHOLDER_PADDING,
                        SLOT_SIZE - (SLOT_PLACEHOLDER_PADDING * 2),
                        SLOT_SIZE - (SLOT_PLACEHOLDER_PADDING * 2)
                    };

                    DrawTexturePro(tex, source, dest, { 0,0 }, 0.0f, Fade(WHITE, 0.3f));
                }
            }
            else {
                DrawText("?", x + 15, y + 15, 20, Fade(WHITE, 0.2f));
            }
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
                Rectangle source = { 0.0f, 0.0f, (float)icon.width, (float)icon.height };

                Rectangle dest = {
                    x + SLOT_PADDING,
                    y + SLOT_PADDING,
                    SLOT_SIZE - (SLOT_PADDING * 2),
                    SLOT_SIZE - (SLOT_PADDING * 2)
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

        float backpackX = INV_START_X + 240;
        float backpackY = INV_START_Y + 50;

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

        float cx = INV_START_X + 110;
        float cy = INV_START_Y + 50;
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