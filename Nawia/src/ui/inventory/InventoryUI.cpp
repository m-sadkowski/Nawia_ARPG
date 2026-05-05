#include "InventoryUI.h"

#include <ResourceManager.h>
#include <UIDefines.h>
#include <UIRenderUtils.h>

#include <string>

namespace Nawia::UI
{

    InventoryUI::InventoryUI() {}

    void InventoryUI::loadResources(Core::ResourceManager& resource_manager)
    {
        _placeholders[Item::EquipmentSlot::Head] = resource_manager.getTexture("assets/textures/ui/slot_head.png");
        _placeholders[Item::EquipmentSlot::Chest] = resource_manager.getTexture("assets/textures/ui/slot_chest.png");
        _placeholders[Item::EquipmentSlot::Legs] = resource_manager.getTexture("assets/textures/ui/slot_legs.png");
        _placeholders[Item::EquipmentSlot::Feet] = resource_manager.getTexture("assets/textures/ui/slot_feet.png");
        _placeholders[Item::EquipmentSlot::Weapon] = resource_manager.getTexture("assets/textures/ui/slot_weapon.png");
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
        const float slot_size = Core::GlobalScaling::scaled(SLOT_SIZE);
        const float slot_spacing = Core::GlobalScaling::scaled(SLOT_SPACING);
        const float equipment_width = Core::GlobalScaling::scaled(EQ_WIDTH);
        const float backpack_section_width = inventory_rect.width - equipment_width;
        const float grid_width = (slot_size * BACKPACK_COLUMNS) + (slot_spacing * (BACKPACK_COLUMNS - 1));
        const float backpack_x = inventory_rect.x + equipment_width + (backpack_section_width - grid_width) / 2.0f;
        const float backpack_y = inventory_rect.y + Core::GlobalScaling::scaled(BP_START_TOP + 5.0f);
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
        const float slot_size = Core::GlobalScaling::scaled(SLOT_SIZE);
        const float padding = Core::GlobalScaling::scaled(PADDING);
        const float equipment_width = Core::GlobalScaling::scaled(EQ_WIDTH);
        const float equipment_center_x = inventory_rect.x + equipment_width / 2.0f;
        const float equipment_top_y = inventory_rect.y + Core::GlobalScaling::scaled(EQ_START_TOP + 5.0f);

        switch (slot_type)
        {
            case Item::EquipmentSlot::Head:
                return { equipment_center_x - slot_size / 2.0f, equipment_top_y, slot_size, slot_size };
            case Item::EquipmentSlot::Neck:
                return { equipment_center_x + slot_size / 2.0f + padding, equipment_top_y, slot_size, slot_size };
            case Item::EquipmentSlot::Chest:
                return { equipment_center_x - slot_size / 2.0f, equipment_top_y + slot_size + padding, slot_size, slot_size };
            case Item::EquipmentSlot::Legs:
                return { equipment_center_x - slot_size / 2.0f, equipment_top_y + (slot_size + padding) * 2.0f, slot_size, slot_size };
            case Item::EquipmentSlot::Feet:
                return { equipment_center_x - slot_size / 2.0f, equipment_top_y + (slot_size + padding) * 3.0f, slot_size, slot_size };
            case Item::EquipmentSlot::Weapon:
                return { equipment_center_x - slot_size * 1.5f - padding, equipment_top_y + slot_size + padding, slot_size, slot_size };
            case Item::EquipmentSlot::OffHand:
                return { equipment_center_x + slot_size / 2.0f + padding, equipment_top_y + slot_size + padding, slot_size, slot_size };
            case Item::EquipmentSlot::Ring:
                return { equipment_center_x - slot_size * 1.5f - padding, equipment_top_y + (slot_size + padding) * 2.0f, slot_size, slot_size };
            case Item::EquipmentSlot::None:
            default:
                return { 0.0f, 0.0f, 0.0f, 0.0f };
        }
    }

    void InventoryUI::render(const Font& font, const Entity::Player& player) const
    {
        const float font_size = Core::GlobalScaling::scaled(FONT_SIZE);
        const Rectangle inventory_rect = getInventoryRect();
        const float text_y_offset = Core::GlobalScaling::scaled(TEXT_PADDING_TOP + 10.0f); // Dodatkowy oddech nad naglowkiem.
        const float equipment_width = Core::GlobalScaling::scaled(EQ_WIDTH);
        const float gold_y_offset = Core::GlobalScaling::scaled(GOLD_PADDING_BOTTOM + 15.0f); // Odstep od dolnej krawedzi panelu.

        // Tlo panelu ekwipunku.
        DrawRectangleRec(inventory_rect, withAlpha(COLOR_PANEL_BG, 0.98f));
        DrawRectangleLinesEx(inventory_rect, 2.0f, withAlpha(COLOR_ACCENT, 0.8f));

        // Separator miedzy wyposazeniem a plecakiem.
        DrawLineEx(
            { inventory_rect.x + equipment_width, inventory_rect.y },
            { inventory_rect.x + equipment_width, inventory_rect.y + inventory_rect.height },
            1.0f,
            withAlpha(COLOR_ACCENT, 0.3f));

        // Naglowki sekcji.
        auto draw_centered_title = [&](const char* text, float start_x, float width)
        {
            const Vector2 text_size = MeasureTextEx(font, text, font_size, 1.0f);
            DrawTextEx(font, text, { start_x + (width - text_size.x) / 2.0f, inventory_rect.y + text_y_offset }, font_size, 1.0f, COLOR_ACCENT);
        };

        draw_centered_title("EKWIPUNEK", inventory_rect.x, equipment_width);
        draw_centered_title("PLECAK", inventory_rect.x + equipment_width, inventory_rect.width - equipment_width);

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
        const float backpack_section_width = inventory_rect.width - equipment_width;

        std::shared_ptr<Item::Item> item_tooltip = nullptr;
        Vector2 tooltip_pos = { 0.0f, 0.0f };

        for (int i = 0; i < BACKPACK_SLOT_COUNT; ++i)
        {
            const Rectangle slot_rect = getBackpackSlotRect(i);
            const bool is_hovered = CheckCollisionPointRec(mouse_pos, slot_rect);
            const std::shared_ptr<Item::Item> item = (i < static_cast<int>(backpack_items.size())) ? backpack_items[i] : nullptr;

            drawSlot(i, slot_rect, is_hovered, item);

            if (is_hovered && item != nullptr)
            {
                item_tooltip = item;
                tooltip_pos = { mouse_pos.x + 15.0f, mouse_pos.y + 15.0f };
            }
        }

        const std::string gold_text = "ZLOTO: " + std::to_string(player.getGold());
        const Vector2 gold_size = MeasureTextEx(font, gold_text.c_str(), font_size, 1.0f);
        DrawTextEx(font, gold_text.c_str(), { inventory_rect.x + equipment_width + (backpack_section_width - gold_size.x) / 2.0f, inventory_rect.y + inventory_rect.height - gold_y_offset }, font_size, 1.0f, COLOR_ACCENT);
    
        if (item_tooltip != nullptr)
            drawTooltip(font, item_tooltip, tooltip_pos.x, tooltip_pos.y);
    }

    void InventoryUI::drawSpecificSlot(const Item::EquipmentSlot slot_type, const Entity::Player& player, const Vector2 mouse_pos) const
    {
        const float placeholder_padding = Core::GlobalScaling::scaled(SLOT_PLACEHOLDER_PADDING);
        const Rectangle slot_rect = getEquipmentSlotRect(slot_type);
        
        const auto item = player.getEquipment().getItemAt(slot_type);
        const bool is_hovered = CheckCollisionPointRec(mouse_pos, slot_rect);

        drawSlot(-1, slot_rect, is_hovered, item);

        if (item == nullptr)
        {
            if (_placeholders.count(slot_type))
            {
                const auto tex_ptr = _placeholders.at(slot_type);
                if (tex_ptr && tex_ptr->id > 0)
                {
                    const Rectangle dest = {
                        slot_rect.x + placeholder_padding,
                        slot_rect.y + placeholder_padding,
                        slot_rect.width - (placeholder_padding * 2.0f),
                        slot_rect.height - (placeholder_padding * 2.0f)
                    };
                    DrawTexturePro(*tex_ptr, { 0, 0, static_cast<float>(tex_ptr->width), static_cast<float>(tex_ptr->height) }, dest, { 0,0 }, 0.0f, withAlpha(WHITE, 0.2f));
                }
            }
        }
    }

    void InventoryUI::drawSlot([[maybe_unused]] const int index, const Rectangle slot_rect, const bool is_hovered, const std::shared_ptr<Item::Item>& item) const
    {
        const float slot_padding = Core::GlobalScaling::scaled(SLOT_PADDING);

        const Color background_color = is_hovered ? withAlpha(COLOR_ACCENT, 0.2f) : withAlpha(BLACK, 0.4f);
        const Color border_color = is_hovered ? COLOR_ACCENT : withAlpha(WHITE, 0.3f);
        
        DrawRectangleRec(slot_rect, background_color);
        DrawRectangleLinesEx(slot_rect, 1.0f, border_color);

        if (item != nullptr)
        {
            const Texture2D icon = item->getIcon();
            if (icon.id > 0)
            {
                const Rectangle dest = {
                    slot_rect.x + slot_padding,
                    slot_rect.y + slot_padding,
                    slot_rect.width - (slot_padding * 2.0f),
                    slot_rect.height - (slot_padding * 2.0f)
                };
                DrawTexturePro(icon, { 0.0f, 0.0f, static_cast<float>(icon.width), static_cast<float>(icon.height) }, dest, { 0, 0 }, 0.0f, WHITE);
            }
        }
    }

    void InventoryUI::drawTooltip(const Font& font, const std::shared_ptr<Item::Item>& item, float x, float y) const
    {
        const float font_size = Core::GlobalScaling::scaled(FONT_SIZE);
        const char* item_name = item->getName().c_str();

        const Vector2 text_size = MeasureTextEx(font, item_name, font_size, 1.0f);
        const float padding = 12.0f;

        DrawRectangleRec({ x, y, text_size.x + (padding * 2.0f), text_size.y + (padding * 2.0f) }, withAlpha(COLOR_PANEL_BG, 0.98f));
        DrawRectangleLinesEx({ x, y, text_size.x + (padding * 2.0f), text_size.y + (padding * 2.0f) }, 1.0f, COLOR_ACCENT);

        DrawTextEx(font, item_name, { x + padding, y + padding }, font_size, 1.0f, COLOR_GOLDEN_TEXT);
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
