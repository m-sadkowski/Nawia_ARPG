#include "InventoryUI.h"
#include "ResourceManager.h"
#include <string>

namespace Nawia::UI {

    InventoryUI::InventoryUI() {}

    void InventoryUI::loadResources(Core::ResourceManager& resourceManager) {
        _placeholders[Item::EquipmentSlot::Head] = resourceManager.getTexture("../assets/textures/ui/slot_head.png");
        _placeholders[Item::EquipmentSlot::Chest] = resourceManager.getTexture("../assets/textures/ui/slot_chest.png");
        _placeholders[Item::EquipmentSlot::Legs] = resourceManager.getTexture("../assets/textures/ui/slot_legs.png");
        _placeholders[Item::EquipmentSlot::Feet] = resourceManager.getTexture("../assets/textures/ui/slot_feet.png");
        _placeholders[Item::EquipmentSlot::Weapon] = resourceManager.getTexture("../assets/textures/ui/slot_weapon.png");
    }

    void InventoryUI::render(const Font& font, const Entity::Player& player) const {
        const float _font_size = Core::GlobalScaling::scaled(FONT_SIZE);
        const float _slot_size = Core::GlobalScaling::scaled(SLOT_SIZE);
        const float _padding = Core::GlobalScaling::scaled(PADDING);
        const float _inv_start_x = Core::GlobalScaling::scaled(INV_START_X);
        const float _inv_start_y = Core::GlobalScaling::scaled(INV_START_Y);
        const float _inv_width = Core::GlobalScaling::scaled(INV_WIDTH);
        const float _inv_height = Core::GlobalScaling::scaled(INV_HEIGHT);
        const float _text_padding_left = Core::GlobalScaling::scaled(TEXT_PADDING_LEFT);
        const float _text_padding_top = Core::GlobalScaling::scaled(TEXT_PADDING_TOP);
        const float _eq_width = Core::GlobalScaling::scaled(EQ_WIDTH);
        const float _eq_start_top = Core::GlobalScaling::scaled(EQ_START_TOP);
        const float _bp_start_top = Core::GlobalScaling::scaled(BP_START_TOP);
        const float _gold_padding_bottom = Core::GlobalScaling::scaled(GOLD_PADDING_BOTTOM);

        // background
        DrawRectangle(_inv_start_x, _inv_start_y, _inv_width, _inv_height, Fade(BLACK, 0.9f));
        DrawRectangleLines(_inv_start_x, _inv_start_y, _inv_width, _inv_height, WHITE);

        DrawLine(_inv_start_x + _eq_width, _inv_start_y, _inv_start_x + _eq_width, _inv_start_y + _inv_height, WHITE);

        DrawTextEx(font, "EQUIPMENT", { _inv_start_x + _text_padding_left, _inv_start_y + _text_padding_top }, _font_size, 1.0f, WHITE);
        DrawTextEx(font, "BACKPACK", { _inv_start_x + _eq_width + _text_padding_left, _inv_start_y + _text_padding_top }, _font_size, 1.0f, WHITE);

        Vector2 mousePos = GetMousePosition();

        float eqCenterX = _inv_start_x + _eq_width/2;
        float eqTopY = _inv_start_y + _eq_start_top;

        drawSpecificSlot(Item::EquipmentSlot::Head, eqCenterX - _slot_size/2, eqTopY, player, mousePos);
        drawSpecificSlot(Item::EquipmentSlot::Neck, eqCenterX + _slot_size/2 + _padding, eqTopY, player, mousePos);
        drawSpecificSlot(Item::EquipmentSlot::Chest, eqCenterX - _slot_size / 2, eqTopY + _slot_size + _padding, player, mousePos);
        drawSpecificSlot(Item::EquipmentSlot::Legs, eqCenterX - _slot_size / 2, eqTopY + (_slot_size + _padding)*2, player, mousePos);
        drawSpecificSlot(Item::EquipmentSlot::Feet, eqCenterX - _slot_size / 2, eqTopY + (_slot_size + _padding) * 3, player, mousePos);
        drawSpecificSlot(Item::EquipmentSlot::Weapon, eqCenterX - _slot_size*1.5f - _padding, eqTopY + _slot_size + _padding, player, mousePos);
        drawSpecificSlot(Item::EquipmentSlot::OffHand, eqCenterX + _slot_size/2 + _padding, eqTopY + _slot_size + _padding, player, mousePos);
        drawSpecificSlot(Item::EquipmentSlot::Ring, eqCenterX - _slot_size * 1.5f - _padding, eqTopY + (_slot_size + _padding) * 2, player, mousePos);


        const auto& backpackItems = player.getBackpack().getItems();
        float backpackX = _inv_start_x + _eq_width + _text_padding_left;
        float backpackY = _inv_start_y + _bp_start_top;

        for (int i = 0; i < 20; ++i) {
            int col = i % 4;
            int row = i / 4;

            float slotX = backpackX + (col * (_slot_size + Core::GlobalScaling::scaled(10.0f)));
            float slotY = backpackY + (row * (_slot_size + Core::GlobalScaling::scaled(10.0f)));

            bool isHovered = CheckCollisionPointRec(mousePos, { slotX, slotY, _slot_size, _slot_size });

            std::shared_ptr<Item::Item> item = (i < backpackItems.size()) ? backpackItems[i] : nullptr;

            drawSlot(i, slotX, slotY, isHovered, item);
        }

        std::string goldText = "GOLD: " + std::to_string(player.getGold());
        DrawTextEx(font, goldText.c_str(), { _inv_start_x + _eq_width + _text_padding_left, _inv_start_y + _inv_height - _gold_padding_bottom }, _font_size, 1.0f, GOLD);
    }

    void InventoryUI::drawSpecificSlot(Item::EquipmentSlot slotType, float x, float y, const Entity::Player& player, Vector2 mousePos) const {
        const float _slot_placeholder_padding = Core::GlobalScaling::scaled(SLOT_PLACEHOLDER_PADDING);
        const float _slot_size = Core::GlobalScaling::scaled(SLOT_SIZE);
        const float _font_size = Core::GlobalScaling::scaled(FONT_SIZE);

        auto item = player.getEquipment().getItemAt(slotType);

        bool isHovered = CheckCollisionPointRec(mousePos, { x, y, _slot_size, _slot_size });

        drawSlot(-1, x, y, isHovered, item);

        if (item == nullptr) {
            if (_placeholders.count(slotType)) {
                auto texPtr = _placeholders.at(slotType);

                if (texPtr && texPtr->id > 0) {
                    Texture2D tex = *texPtr;

                    Rectangle source = { 0, 0, (float)tex.width, (float)tex.height };
                    Rectangle dest = {
                        x + _slot_placeholder_padding,
                        y + _slot_placeholder_padding,
                        _slot_size - (_slot_placeholder_padding * 2),
                        _slot_size - (_slot_placeholder_padding * 2)
                    };

                    DrawTexturePro(tex, source, dest, { 0,0 }, 0.0f, Fade(WHITE, 0.3f));
                }
            }
            else {
                DrawText("?", x + Core::GlobalScaling::scaled(15.0f), y + Core::GlobalScaling::scaled(15.0f), _font_size, Fade(WHITE, 0.2f));
            }
        }
    }

    void InventoryUI::drawSlot(int index, float x, float y, bool isHovered, const std::shared_ptr<Item::Item>& item) const {
        const float _slot_padding = Core::GlobalScaling::scaled(SLOT_PADDING);
        const float _slot_size = Core::GlobalScaling::scaled(SLOT_SIZE);
        // slot background
        Color slotColor = isHovered ? LIGHTGRAY : DARKGRAY;
        DrawRectangle(x, y, _slot_size, _slot_size, slotColor);
        DrawRectangleLines(x, y, _slot_size, _slot_size, WHITE);

        if (item != nullptr) {
            Texture2D icon = item->getIcon();

            // check if texture is loaded
            if (icon.id > 0) {
                Rectangle source = { 0.0f, 0.0f, (float)icon.width, (float)icon.height };

                Rectangle dest = {
                    x + _slot_padding,
                    y + _slot_padding,
                    _slot_size - (_slot_padding * 2),
                    _slot_size - (_slot_padding * 2)
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
        const float _slot_size = Core::GlobalScaling::scaled(SLOT_SIZE);
        const float _inv_start_x = Core::GlobalScaling::scaled(INV_START_X);
        const float _inv_start_y = Core::GlobalScaling::scaled(INV_START_Y);

        if (!IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            return -1;
        }

        Vector2 mousePos = GetMousePosition();

        float backpackX = _inv_start_x + Core::GlobalScaling::scaled(240.0f);
        float backpackY = _inv_start_y + Core::GlobalScaling::scaled(50.0f);

        for (int i = 0; i < 20; ++i) {
            int col = i % 4;
            int row = i / 4;
            float slotX = backpackX + (col * (_slot_size + Core::GlobalScaling::scaled(10.0f)));
            float slotY = backpackY + (row * (_slot_size + Core::GlobalScaling::scaled(10.0f)));

            Rectangle slotRect = { slotX, slotY, _slot_size, _slot_size };

            if (CheckCollisionPointRec(mousePos, slotRect)) {
                return i;
            }
        }

        return -1;
    }

    Item::EquipmentSlot InventoryUI::getClickedEquipmentSlot() const {
        const float _inv_start_x = Core::GlobalScaling::scaled(INV_START_X);
        const float _inv_start_y = Core::GlobalScaling::scaled(INV_START_Y);
        const float _slot_size = Core::GlobalScaling::scaled(SLOT_SIZE);
        const float _padding = Core::GlobalScaling::scaled(PADDING);

        if (!IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            return Item::EquipmentSlot::None;
        }

        Vector2 mousePos = GetMousePosition();

        float cx = _inv_start_x + Core::GlobalScaling::scaled(110.0f);
        float cy = _inv_start_y + Core::GlobalScaling::scaled(50.0f);
        float size = _slot_size;

        struct SlotHitbox {
            Item::EquipmentSlot slot;
            float x;
            float y;
        };

        SlotHitbox hitboxes[] = {
            { Item::EquipmentSlot::Head,    cx - _slot_size / 2, cy },
            { Item::EquipmentSlot::Neck,    cx + _slot_size / 2 + _padding, cy },
            { Item::EquipmentSlot::Chest,   cx - _slot_size / 2, cy + _slot_size + _padding },
            { Item::EquipmentSlot::Legs,    cx - _slot_size / 2, cy + (_slot_size + _padding) * 2 },
            { Item::EquipmentSlot::Feet,    cx - _slot_size / 2, cy + (_slot_size + _padding) * 3 },
            { Item::EquipmentSlot::Weapon,  cx - _slot_size * 1.5f - _padding, cy + _slot_size + _padding },
            { Item::EquipmentSlot::OffHand, cx + _slot_size / 2 + _padding, cy + _slot_size + _padding },
            { Item::EquipmentSlot::Ring,    cx - _slot_size * 1.5f - _padding, cy + (_slot_size + _padding) * 2 }
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