#include "InventoryUI.h"
#include <ResourceManager.h>
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

        const Vector2 mouse_pos = GetMousePosition();

        const float eq_center_x = _inv_start_x + _eq_width/2;
        const float eq_top_y = _inv_start_y + _eq_start_top;

        drawSpecificSlot(Item::EquipmentSlot::Head, eq_center_x - _slot_size/2, eq_top_y, player, mouse_pos);
        drawSpecificSlot(Item::EquipmentSlot::Neck, eq_center_x + _slot_size/2 + _padding, eq_top_y, player, mouse_pos);
        drawSpecificSlot(Item::EquipmentSlot::Chest, eq_center_x - _slot_size / 2, eq_top_y + _slot_size + _padding, player, mouse_pos);
        drawSpecificSlot(Item::EquipmentSlot::Legs, eq_center_x - _slot_size / 2, eq_top_y + (_slot_size + _padding)*2, player, mouse_pos);
        drawSpecificSlot(Item::EquipmentSlot::Feet, eq_center_x - _slot_size / 2, eq_top_y + (_slot_size + _padding) * 3, player, mouse_pos);
        drawSpecificSlot(Item::EquipmentSlot::Weapon, eq_center_x - _slot_size*1.5f - _padding, eq_top_y + _slot_size + _padding, player, mouse_pos);
        drawSpecificSlot(Item::EquipmentSlot::OffHand, eq_center_x + _slot_size/2 + _padding, eq_top_y + _slot_size + _padding, player, mouse_pos);
        drawSpecificSlot(Item::EquipmentSlot::Ring, eq_center_x - _slot_size * 1.5f - _padding, eq_top_y + (_slot_size + _padding) * 2, player, mouse_pos);


        const auto& backpack_items = player.getBackpack().getItems();
        const float backpack_x = _inv_start_x + _eq_width + _text_padding_left;
        const float backpack_y = _inv_start_y + _bp_start_top;

        std::shared_ptr<Item::Item> item_tooltip = nullptr;
        Vector2 tooltip_pos = { 0, 0 };

        for (int i = 0; i < 20; ++i) {
            const int col = i % 4;
            const int row = i / 4;

            const float slot_x = backpack_x + (col * (_slot_size + Core::GlobalScaling::scaled(10.0f)));
            const float slot_y = backpack_y + (row * (_slot_size + Core::GlobalScaling::scaled(10.0f)));

            const bool is_hovered = CheckCollisionPointRec(mouse_pos, { slot_x, slot_y, _slot_size, _slot_size });

            std::shared_ptr<Item::Item> item = (i < backpack_items.size()) ? backpack_items[i] : nullptr;

            drawSlot(i, slot_x, slot_y, is_hovered, item);

            if (is_hovered && item != nullptr) {
                item_tooltip = item;
                tooltip_pos = { mouse_pos.x + 15, mouse_pos.y + 15 };
            }
        }

        std::string gold_text = "GOLD: " + std::to_string(player.getGold());
        DrawTextEx(font, gold_text.c_str(), { _inv_start_x + _eq_width + _text_padding_left, _inv_start_y + _inv_height - _gold_padding_bottom }, _font_size, 1.0f, GOLD);
    
        // drawing tooltip
        if (item_tooltip != nullptr) {
            drawTooltip(font, item_tooltip, tooltip_pos.x, tooltip_pos.y);
        }
    }

    void InventoryUI::drawSpecificSlot(Item::EquipmentSlot slotType, float x, float y, const Entity::Player& player, Vector2 mousePos) const {
        const float _slot_placeholder_padding = Core::GlobalScaling::scaled(SLOT_PLACEHOLDER_PADDING);
        const float _slot_size = Core::GlobalScaling::scaled(SLOT_SIZE);
        const float _font_size = Core::GlobalScaling::scaled(FONT_SIZE);

        const auto item = player.getEquipment().getItemAt(slotType);

        const bool is_hovered = CheckCollisionPointRec(mousePos, { x, y, _slot_size, _slot_size });

        drawSlot(-1, x, y, is_hovered, item);

        if (item == nullptr) {
            if (_placeholders.count(slotType)) {
                const auto tex_ptr = _placeholders.at(slotType);

                if (tex_ptr && tex_ptr->id > 0) {
                    Texture2D tex = *tex_ptr;

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
        const float _font_size = Core::GlobalScaling::scaled(FONT_SIZE);
        const float _padding = Core::GlobalScaling::scaled(PADDING);

        // slot background
        const Color slot_color = isHovered ? LIGHTGRAY : DARKGRAY;
        DrawRectangle(x, y, _slot_size, _slot_size, slot_color);
        DrawRectangleLines(x, y, _slot_size, _slot_size, WHITE);

        if (item != nullptr) {
            const Texture2D icon = item->getIcon();

            // check if texture is loaded
            if (icon.id > 0) {
                const Rectangle source = { 0.0f, 0.0f, (float)icon.width, (float)icon.height };

                const Rectangle dest = {
                    x + _slot_padding,
                    y + _slot_padding,
                    _slot_size - (_slot_padding * 2),
                    _slot_size - (_slot_padding * 2)
                };

                DrawTexturePro(icon, source, dest, { 0, 0 }, 0.0f, WHITE);
            }
        }
    }

    void InventoryUI::drawTooltip(const Font& font, const std::shared_ptr<Item::Item>& item, float x, float y) const {
        const float _font_size = Core::GlobalScaling::scaled(FONT_SIZE);

        const char* _item_name = item->getName().c_str();

        const Vector2 text_size = MeasureTextEx(font, _item_name, _font_size, 1.0f);
        const float _padding = 8.0f;

        DrawRectangle(x, y, text_size.x + (_padding * 2), text_size.y + (_padding * 2), Fade(BLACK, 0.9f));
        DrawRectangleLines(x, y, text_size.x + (_padding * 2), text_size.y + (_padding * 2), WHITE);

        DrawTextEx(font, _item_name, { x + _padding, y + _padding }, _font_size, 1.0f, YELLOW);
    }

    int InventoryUI::handleInput() const {
        const float _slot_size = Core::GlobalScaling::scaled(SLOT_SIZE);
        const float _inv_start_x = Core::GlobalScaling::scaled(INV_START_X);
        const float _inv_start_y = Core::GlobalScaling::scaled(INV_START_Y);

        if (!IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            return -1;
        }

        const Vector2 mouse_pos = GetMousePosition();

        const float backpack_x = _inv_start_x + Core::GlobalScaling::scaled(240.0f);
        const float backpack_y = _inv_start_y + Core::GlobalScaling::scaled(50.0f);

        for (int i = 0; i < 20; ++i) {
            const int col = i % 4;
            const int row = i / 4;
            const float slot_x = backpack_x + (col * (_slot_size + Core::GlobalScaling::scaled(10.0f)));
            const float slot_y = backpack_y + (row * (_slot_size + Core::GlobalScaling::scaled(10.0f)));

            const Rectangle slot_rect = { slot_x, slot_y, _slot_size, _slot_size };

            if (CheckCollisionPointRec(mouse_pos, slot_rect)) {
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

        const Vector2 mouse_pos = GetMousePosition();

        const float cx = _inv_start_x + Core::GlobalScaling::scaled(110.0f);
        const float cy = _inv_start_y + Core::GlobalScaling::scaled(50.0f);
        const float size = _slot_size;

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
            const Rectangle rect = { hb.x, hb.y, size, size };

            if (CheckCollisionPointRec(mouse_pos, rect)) {
                return hb.slot;
            }
        }

        return Item::EquipmentSlot::None;
    }

}