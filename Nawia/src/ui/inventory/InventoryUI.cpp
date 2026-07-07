#include "InventoryUI.h"

#include <InventoryRenderUtils.h>
#include <ResourceManager.h>
#include <UIDefines.h>
#include <UIRenderUtils.h>

#include <string>

namespace Nawia::UI
{
    InventoryUI::InventoryUI() {}

    void InventoryUI::loadResources(Core::ResourceManager& resource_manager)
    {
        _background = resource_manager.getTexture("assets/textures/ui/eq.png");

        smoothUiTexture(_background);
    }

    Rectangle InventoryUI::getInventoryRect() const
    {
        return {
            Core::GlobalScaling::scaled(50.0f),
            Core::GlobalScaling::scaled(50.0f),
            Core::GlobalScaling::scaled(INV_WIDTH),
            Core::GlobalScaling::scaled(INV_HEIGHT)
        };
    }

    Rectangle InventoryUI::getBackpackSlotRect(const int index) const
    {
        const Rectangle inventory_rect = getInventoryRect();
        const float slot_size = inventory_rect.width * 0.072f;
        const float slot_spacing = inventory_rect.width * 0.0075f;
        const float backpack_x = inventory_rect.x + inventory_rect.width * 0.433f;
        const float backpack_y = inventory_rect.y + inventory_rect.height * 0.148f;
        const int column = index % BACKPACK_COLUMNS;
        const int row = index / BACKPACK_COLUMNS;

        return {
            backpack_x + column * (slot_size + slot_spacing),
            backpack_y + row * (slot_size + slot_spacing),
            slot_size,
            slot_size
        };
    }

    Rectangle InventoryUI::getEquipmentSlotRect(const Item::EquipmentSlot slot_type) const
    {
        const Rectangle inventory_rect = getInventoryRect();
        auto slot_from_texture = [&](const float texture_x, const float texture_y, const float size_ratio)
        {
            const float slot_size = inventory_rect.width * size_ratio;
            return Rectangle{
                inventory_rect.x + inventory_rect.width * texture_x,
                inventory_rect.y + inventory_rect.height * texture_y,
                slot_size,
                slot_size
            };
        };

        switch (slot_type)
        {
            case Item::EquipmentSlot::Head:
                return slot_from_texture(0.150f, 0.138f, 0.070f);
            case Item::EquipmentSlot::Neck:
                return slot_from_texture(0.254f, 0.176f, 0.046f);
            case Item::EquipmentSlot::Chest:
                return slot_from_texture(0.150f, 0.278f, 0.070f);
            case Item::EquipmentSlot::Legs:
                return slot_from_texture(0.150f, 0.420f, 0.070f);
            case Item::EquipmentSlot::Feet:
                return slot_from_texture(0.150f, 0.560f, 0.070f);
            case Item::EquipmentSlot::Weapon:
                return slot_from_texture(0.150f, 0.700f, 0.070f);
            case Item::EquipmentSlot::OffHand:
                return slot_from_texture(0.310f, 0.176f, 0.053f);
            case Item::EquipmentSlot::Ring:
                return slot_from_texture(0.254f, 0.313f, 0.046f);
            case Item::EquipmentSlot::None:
            default:
                return { 0.0f, 0.0f, 0.0f, 0.0f };
        }
    }

    void InventoryUI::render(const Font& font, const Entity::Player& player) const
    {
        const float font_size = Core::GlobalScaling::scaled(FONT_SIZE);
        const Rectangle inventory_rect = getInventoryRect();
        const float gold_y_offset = Core::GlobalScaling::scaled(GOLD_PADDING_BOTTOM + 30.0f); // Odstep od dolnej krawedzi panelu.

        if (_background && _background->id > 0)
        {
            DrawTexturePro(
                *_background,
                { 0.0f, 0.0f, static_cast<float>(_background->width), static_cast<float>(_background->height) },
                inventory_rect,
                { 0.0f, 0.0f },
                0.0f,
                WHITE);
        }
        else
        {
            drawPanelFrame(inventory_rect);
        }

        const Vector2 mouse_pos = GetMousePosition();
        drawSpecificSlot(Item::EquipmentSlot::Head, player, mouse_pos);
        drawSpecificSlot(Item::EquipmentSlot::Neck, player, mouse_pos);
        drawSpecificSlot(Item::EquipmentSlot::Chest, player, mouse_pos);
        drawSpecificSlot(Item::EquipmentSlot::Legs, player, mouse_pos);
        drawSpecificSlot(Item::EquipmentSlot::Feet, player, mouse_pos);
        drawSpecificSlot(Item::EquipmentSlot::Weapon, player, mouse_pos);
        drawSpecificSlot(Item::EquipmentSlot::OffHand, player, mouse_pos);
        drawSpecificSlot(Item::EquipmentSlot::Ring, player, mouse_pos);

        const auto& backpack_items = player.getBackpack().getItems();

        std::shared_ptr<Item::Item> item_tooltip = nullptr;
        Vector2 tooltip_pos = { 0.0f, 0.0f };

        for (int i = 0; i < BACKPACK_SLOT_COUNT; ++i)
        {
            const Rectangle slot_rect = getBackpackSlotRect(i);
            const bool is_hovered = CheckCollisionPointRec(mouse_pos, slot_rect);
            const std::shared_ptr<Item::Item> item = (i < static_cast<int>(backpack_items.size())) ? backpack_items[i] : nullptr;

            InventoryRender::drawItemSlot(slot_rect, is_hovered, item, SLOT_PADDING);

            if (is_hovered && item != nullptr)
            {
                item_tooltip = item;
                tooltip_pos = { mouse_pos.x + 15.0f, mouse_pos.y + 15.0f };
            }
        }

        const std::string gold_text = "ZLOTO: " + std::to_string(player.getGold());
        drawCenteredText(font, gold_text.c_str(), { inventory_rect.x, inventory_rect.y + inventory_rect.height - gold_y_offset, inventory_rect.width, font_size }, font_size, 1.0f, COLOR_ACCENT);
    
        if (item_tooltip != nullptr)
            InventoryRender::drawItemTooltip(font, item_tooltip, tooltip_pos.x, tooltip_pos.y, font_size);
    }

    void InventoryUI::drawSpecificSlot(const Item::EquipmentSlot slot_type, const Entity::Player& player, const Vector2 mouse_pos) const
    {
        const Rectangle slot_rect = getEquipmentSlotRect(slot_type);
        
        const auto item = player.getEquipment().getItemAt(slot_type);
        const bool is_hovered = CheckCollisionPointRec(mouse_pos, slot_rect);

        InventoryRender::drawItemSlot(slot_rect, is_hovered, item, SLOT_PADDING);

        if (item == nullptr)
            DrawRectangleLinesEx(slot_rect, 1.0f, withAlpha(RAYWHITE, 0.10f));
    }

    int InventoryUI::handleInput() const
    {
        if (!IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
            return -1;

        const Vector2 mouse_pos = GetMousePosition();

        for (int i = 0; i < BACKPACK_SLOT_COUNT; ++i)
        {
            if (CheckCollisionPointRec(mouse_pos, getBackpackSlotRect(i)))
                return i;
        }

        return -1;
    }

    Item::EquipmentSlot InventoryUI::getClickedEquipmentSlot() const
    {
        if (!IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
            return Item::EquipmentSlot::None;

        const Vector2 mouse_pos = GetMousePosition();

        struct SlotDefinition { Item::EquipmentSlot slot; };
        SlotDefinition slots[] = {
            { Item::EquipmentSlot::Head },
            { Item::EquipmentSlot::Neck },
            { Item::EquipmentSlot::Chest },
            { Item::EquipmentSlot::Legs },
            { Item::EquipmentSlot::Feet },
            { Item::EquipmentSlot::Weapon },
            { Item::EquipmentSlot::OffHand },
            { Item::EquipmentSlot::Ring }
        };

        for (const auto& slot_definition : slots)
        {
            if (CheckCollisionPointRec(mouse_pos, getEquipmentSlotRect(slot_definition.slot)))
                return slot_definition.slot;
        }

        return Item::EquipmentSlot::None;
    }
} // namespace Nawia::UI
