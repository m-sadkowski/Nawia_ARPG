#include "ChestUI.h"

#include <GlobalScaling.h>
#include <InventoryRenderUtils.h>
#include <ResourceManager.h>
#include <UIDefines.h>
#include <UIRenderUtils.h>

namespace Nawia::UI
{
    ChestUI::ChestUI() {}

    void ChestUI::loadResources(Core::ResourceManager& resource_manager)
    {
        _background = resource_manager.getTexture("assets/textures/ui/chest.png");
        smoothUiTexture(_background);
    }

    Rectangle ChestUI::getPanelRect() const
    {
        return {
            Core::GlobalScaling::scaled(INV_START_X),
            Core::GlobalScaling::scaled(INV_START_Y),
            Core::GlobalScaling::scaled(INV_WIDTH),
            Core::GlobalScaling::scaled(INV_HEIGHT)
        };
    }

    Rectangle ChestUI::getSlotRect(const int index) const
    {
        const Rectangle panel_rect = getPanelRect();
        const float slot_size = panel_rect.width * 0.19f;
        const float slot_spacing = panel_rect.width * 0.035f;
        const float backpack_x = panel_rect.x + panel_rect.width * 0.18f;
        const float backpack_y = panel_rect.y + panel_rect.height * 0.18f;
        const int column = index % COLS;
        const int row = index / COLS;

        return {
            backpack_x + column * (slot_size + slot_spacing),
            backpack_y + row * (slot_size + slot_spacing),
            slot_size,
            slot_size
        };
    }

    void ChestUI::render(const Item::Backpack& chest_backpack, const Font& font) const 
    {
        const Rectangle panel_rect = getPanelRect();

        if (_background && _background->id > 0)
        {
            DrawTexturePro(
                *_background,
                { 0.0f, 0.0f, static_cast<float>(_background->width), static_cast<float>(_background->height) },
                panel_rect,
                { 0.0f, 0.0f },
                0.0f,
                WHITE);
        }
        else
        {
            drawPanelFrame(panel_rect, 0.95f);
        }

        const Vector2 mouse_position = GetMousePosition();

        std::shared_ptr<Item::Item> item_tooltip = nullptr;
        Vector2 tooltip_position = { 0.0f, 0.0f };

        for (int i = 0; i < SLOT_AMOUNT; ++i)
        {
            const Rectangle slot_rect = getSlotRect(i);
            const bool is_hovered = CheckCollisionPointRec(mouse_position, slot_rect);
            const std::shared_ptr<Item::Item> item = (i < static_cast<int>(chest_backpack.getItems().size())) ? chest_backpack.getItems()[i] : nullptr;

            InventoryRender::drawItemSlot(slot_rect, is_hovered, item, SLOT_PADDING);

            if (is_hovered && item != nullptr)
            {
                item_tooltip = item;
                tooltip_position = { mouse_position.x + 15.0f, mouse_position.y + 15.0f };
            }
        }

        if (item_tooltip != nullptr)
            InventoryRender::drawItemTooltip(font, item_tooltip, tooltip_position.x, tooltip_position.y, Core::GlobalScaling::scaled(FONT_SIZE));
    }

    int ChestUI::handleInput() const 
    {
        if (!IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
            return -1;

        const Vector2 mouse_position = GetMousePosition();

        for (int i = 0; i < SLOT_AMOUNT; ++i) 
        {
            if (CheckCollisionPointRec(mouse_position, getSlotRect(i)))
                return i;
        }

        return -1;
    }

} // namespace Nawia::UI
