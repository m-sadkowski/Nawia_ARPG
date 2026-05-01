#include "InventoryUI.h"
#include "UIDefines.h"
#include "UIRenderUtils.h"

#include <ResourceManager.h>

#include <string>

namespace Nawia::UI
{

    InventoryUI::InventoryUI() {}

    void InventoryUI::loadResources(Core::ResourceManager& resource_manager)
    {
        _placeholders[Item::EquipmentSlot::Head] = resource_manager.getTexture("../assets/textures/ui/slot_head.png");
        _placeholders[Item::EquipmentSlot::Chest] = resource_manager.getTexture("../assets/textures/ui/slot_chest.png");
        _placeholders[Item::EquipmentSlot::Legs] = resource_manager.getTexture("../assets/textures/ui/slot_legs.png");
        _placeholders[Item::EquipmentSlot::Feet] = resource_manager.getTexture("../assets/textures/ui/slot_feet.png");
        _placeholders[Item::EquipmentSlot::Weapon] = resource_manager.getTexture("../assets/textures/ui/slot_weapon.png");
    }

    void InventoryUI::render(const Font& font, const Entity::Player& player) const
    {
        const float font_size = Core::GlobalScaling::scaled(FONT_SIZE);
        const float slot_size = Core::GlobalScaling::scaled(SLOT_SIZE);
        const float padding = Core::GlobalScaling::scaled(PADDING);
        
        // Moved window further from corners
        const float inv_x = Core::GlobalScaling::scaled(50.0f);
        const float inv_y = Core::GlobalScaling::scaled(50.0f);
        
        const float inv_width = Core::GlobalScaling::scaled(INV_WIDTH);
        const float inv_height = Core::GlobalScaling::scaled(INV_HEIGHT);
        const float text_x_offset = Core::GlobalScaling::scaled(TEXT_PADDING_LEFT);
        const float text_y_offset = Core::GlobalScaling::scaled(TEXT_PADDING_TOP + 10.0f); // More top padding
        const float equipment_width = Core::GlobalScaling::scaled(EQ_WIDTH);
        const float equipment_y_start = Core::GlobalScaling::scaled(EQ_START_TOP + 5.0f);
        const float backpack_y_start = Core::GlobalScaling::scaled(BP_START_TOP + 5.0f);
        const float gold_y_offset = Core::GlobalScaling::scaled(GOLD_PADDING_BOTTOM + 15.0f); // More bottom padding

        // Premium Background
        DrawRectangleRec({ inv_x, inv_y, inv_width, inv_height }, withAlpha(COLOR_PANEL_BG, 0.98f));
        DrawRectangleLinesEx({ inv_x, inv_y, inv_width, inv_height }, 2.0f, withAlpha(COLOR_ACCENT, 0.8f));
        
        // Header separator
        DrawLineEx({ inv_x + equipment_width, inv_y }, { inv_x + equipment_width, inv_y + inv_height }, 1.0f, withAlpha(COLOR_ACCENT, 0.3f));

        // Centered Titles
        auto draw_centered_title = [&](const char* text, float start_x, float width)
        {
            const Vector2 text_size = MeasureTextEx(font, text, font_size, 1.0f);
            DrawTextEx(font, text, { start_x + (width - text_size.x) / 2.0f, inv_y + text_y_offset }, font_size, 1.0f, COLOR_ACCENT);
        };

        draw_centered_title("EKWIPUNEK", inv_x, equipment_width);
        draw_centered_title("PLECAK", inv_x + equipment_width, inv_width - equipment_width);

        const Vector2 mouse_pos = GetMousePosition();
        const float equipment_center_x = inv_x + equipment_width / 2.0f;
        const float equipment_top_y = inv_y + equipment_y_start;

        drawSpecificSlot(Item::EquipmentSlot::Head, equipment_center_x - slot_size / 2.0f, equipment_top_y, player, mouse_pos);
        drawSpecificSlot(Item::EquipmentSlot::Neck, equipment_center_x + slot_size / 2.0f + padding, equipment_top_y, player, mouse_pos);
        drawSpecificSlot(Item::EquipmentSlot::Chest, equipment_center_x - slot_size / 2.0f, equipment_top_y + slot_size + padding, player, mouse_pos);
        drawSpecificSlot(Item::EquipmentSlot::Legs, equipment_center_x - slot_size / 2.0f, equipment_top_y + (slot_size + padding) * 2.0f, player, mouse_pos);
        drawSpecificSlot(Item::EquipmentSlot::Feet, equipment_center_x - slot_size / 2.0f, equipment_top_y + (slot_size + padding) * 3.0f, player, mouse_pos);
        drawSpecificSlot(Item::EquipmentSlot::Weapon, equipment_center_x - slot_size * 1.5f - padding, equipment_top_y + slot_size + padding, player, mouse_pos);
        drawSpecificSlot(Item::EquipmentSlot::OffHand, equipment_center_x + slot_size / 2.0f + padding, equipment_top_y + slot_size + padding, player, mouse_pos);
        drawSpecificSlot(Item::EquipmentSlot::Ring, equipment_center_x - slot_size * 1.5f - padding, equipment_top_y + (slot_size + padding) * 2.0f, player, mouse_pos);

        const auto& backpack_items = player.getBackpack().getItems();
        
        // Center the backpack grid within its section
        const float backpack_section_width = inv_width - equipment_width;
        const float grid_width = (slot_size * 4.0f) + (Core::GlobalScaling::scaled(10.0f) * 3.0f);
        const float backpack_x = inv_x + equipment_width + (backpack_section_width - grid_width) / 2.0f;
        const float backpack_y = inv_y + backpack_y_start;

        std::shared_ptr<Item::Item> item_tooltip = nullptr;
        Vector2 tooltip_pos = { 0.0f, 0.0f };

        for (int i = 0; i < 20; ++i)
        {
            const int column = i % 4;
            const int row = i / 4;

            const float slot_x = backpack_x + (column * (slot_size + Core::GlobalScaling::scaled(10.0f)));
            const float slot_y = backpack_y + (row * (slot_size + Core::GlobalScaling::scaled(10.0f)));

            const bool is_hovered = CheckCollisionPointRec(mouse_pos, { slot_x, slot_y, slot_size, slot_size });
            const std::shared_ptr<Item::Item> item = (i < static_cast<int>(backpack_items.size())) ? backpack_items[i] : nullptr;

            drawSlot(i, slot_x, slot_y, is_hovered, item);

            if (is_hovered && item != nullptr)
            {
                item_tooltip = item;
                tooltip_pos = { mouse_pos.x + 15.0f, mouse_pos.y + 15.0f };
            }
        }

        const std::string gold_text = "ZLOTO: " + std::to_string(player.getGold());
        const Vector2 gold_size = MeasureTextEx(font, gold_text.c_str(), font_size, 1.0f);
        DrawTextEx(font, gold_text.c_str(), { inv_x + equipment_width + (backpack_section_width - gold_size.x) / 2.0f, inv_y + inv_height - gold_y_offset }, font_size, 1.0f, COLOR_ACCENT);
    
        if (item_tooltip != nullptr)
            drawTooltip(font, item_tooltip, tooltip_pos.x, tooltip_pos.y);
    }

    void InventoryUI::drawSpecificSlot(Item::EquipmentSlot slot_type, float x, float y, const Entity::Player& player, Vector2 mouse_pos) const
    {
        const float placeholder_padding = Core::GlobalScaling::scaled(SLOT_PLACEHOLDER_PADDING);
        const float slot_size = Core::GlobalScaling::scaled(SLOT_SIZE);
        
        const auto item = player.getEquipment().getItemAt(slot_type);
        const bool is_hovered = CheckCollisionPointRec(mouse_pos, { x, y, slot_size, slot_size });

        drawSlot(-1, x, y, is_hovered, item);

        if (item == nullptr)
        {
            if (_placeholders.count(slot_type))
            {
                const auto tex_ptr = _placeholders.at(slot_type);
                if (tex_ptr && tex_ptr->id > 0)
                {
                    const Rectangle dest = { x + placeholder_padding, y + placeholder_padding, slot_size - (placeholder_padding * 2.0f), slot_size - (placeholder_padding * 2.0f) };
                    DrawTexturePro(*tex_ptr, { 0, 0, static_cast<float>(tex_ptr->width), static_cast<float>(tex_ptr->height) }, dest, { 0,0 }, 0.0f, withAlpha(WHITE, 0.2f));
                }
            }
        }
    }

    void InventoryUI::drawSlot(int index, float x, float y, bool is_hovered, const std::shared_ptr<Item::Item>& item) const
    {
        const float slot_padding = Core::GlobalScaling::scaled(SLOT_PADDING);
        const float slot_size = Core::GlobalScaling::scaled(SLOT_SIZE);

        const Color background_color = is_hovered ? withAlpha(COLOR_ACCENT, 0.2f) : withAlpha(BLACK, 0.4f);
        const Color border_color = is_hovered ? COLOR_ACCENT : withAlpha(WHITE, 0.3f);
        
        DrawRectangleRec({ x, y, slot_size, slot_size }, background_color);
        DrawRectangleLinesEx({ x, y, slot_size, slot_size }, 1.0f, border_color);

        if (item != nullptr)
        {
            const Texture2D icon = item->getIcon();
            if (icon.id > 0)
            {
                const Rectangle dest = { x + slot_padding, y + slot_padding, slot_size - (slot_padding * 2.0f), slot_size - (slot_padding * 2.0f) };
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
        const float slot_size = Core::GlobalScaling::scaled(SLOT_SIZE);
        const float inv_x = Core::GlobalScaling::scaled(50.0f); // Match render
        const float inv_y = Core::GlobalScaling::scaled(50.0f);
        const float inv_width = Core::GlobalScaling::scaled(INV_WIDTH);
        const float equipment_width = Core::GlobalScaling::scaled(EQ_WIDTH);

        const float backpack_section_width = inv_width - equipment_width;
        const float grid_width = (slot_size * 4.0f) + (Core::GlobalScaling::scaled(10.0f) * 3.0f);
        const float backpack_x = inv_x + equipment_width + (backpack_section_width - grid_width) / 2.0f;
        const float backpack_y = inv_y + Core::GlobalScaling::scaled(BP_START_TOP);

        for (int i = 0; i < 20; ++i)
        {
            const int column = i % 4;
            const int row = i / 4;
            const float slot_x = backpack_x + (column * (slot_size + Core::GlobalScaling::scaled(10.0f)));
            const float slot_y = backpack_y + (row * (slot_size + Core::GlobalScaling::scaled(10.0f)));

            if (CheckCollisionPointRec(mouse_pos, { slot_x, slot_y, slot_size, slot_size }))
                return i;
        }

        return -1;
    }

    Item::EquipmentSlot InventoryUI::getClickedEquipmentSlot() const
    {
        if (!IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
            return Item::EquipmentSlot::None;

        const Vector2 mouse_pos = GetMousePosition();
        const float inv_x = Core::GlobalScaling::scaled(50.0f);
        const float inv_y = Core::GlobalScaling::scaled(50.0f);
        const float slot_size = Core::GlobalScaling::scaled(SLOT_SIZE);
        const float padding = Core::GlobalScaling::scaled(PADDING);

        const float equipment_center_x = inv_x + Core::GlobalScaling::scaled(EQ_WIDTH) / 2.0f;
        const float equipment_top_y = inv_y + Core::GlobalScaling::scaled(EQ_START_TOP);

        struct SlotDefinition { Item::EquipmentSlot slot; float x; float y; };
        SlotDefinition slots[] = {
            { Item::EquipmentSlot::Head,    equipment_center_x - slot_size / 2.0f, equipment_top_y },
            { Item::EquipmentSlot::Neck,    equipment_center_x + slot_size / 2.0f + padding, equipment_top_y },
            { Item::EquipmentSlot::Chest,   equipment_center_x - slot_size / 2.0f, equipment_top_y + slot_size + padding },
            { Item::EquipmentSlot::Legs,    equipment_center_x - slot_size / 2.0f, equipment_top_y + (slot_size + padding) * 2.0f },
            { Item::EquipmentSlot::Feet,    equipment_center_x - slot_size / 2.0f, equipment_top_y + (slot_size + padding) * 3.0f },
            { Item::EquipmentSlot::Weapon,  equipment_center_x - slot_size * 1.5f - padding, equipment_top_y + slot_size + padding },
            { Item::EquipmentSlot::OffHand, equipment_center_x + slot_size / 2.0f + padding, equipment_top_y + slot_size + padding },
            { Item::EquipmentSlot::Ring,    equipment_center_x - slot_size * 1.5f - padding, equipment_top_y + (slot_size + padding) * 2.0f }
        };

        for (const auto& s : slots)
            if (CheckCollisionPointRec(mouse_pos, { s.x, s.y, slot_size, slot_size }))
                return s.slot;

        return Item::EquipmentSlot::None;
    }
} // namespace Nawia::UI