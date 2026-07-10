#include "UIHandler.h"

#include <Backpack.h>
#include <InteractiveClickable.h>
#include <Item.h>
#include <Player.h>
#include <QuestManager.h>

namespace Nawia::UI
{
    namespace
    {
        constexpr int BABA_YAGA_BOOK_ITEM_ID = 18;
    }

    void UIHandler::handleInput()
    {
        if (_dialogueUI.isOpen())
        {
            _dialogueUI.handleInput();
            return;
        }

        if (IsKeyPressed(KEY_I) || IsKeyPressed(KEY_TAB))
        {
            if (_current_container)
                closeContainer();
            toggleInventory();
        }

        if (IsKeyPressed(KEY_P) && !_current_container)
            toggleQuestUI();
        if (_is_quest_ui_open)
            _quest_ui->handleInput();

        if (_is_inventory_open && handleInventoryPanelInput())
            return;
    }

    bool UIHandler::handleInventoryPanelInput()
    {
        const int backpack_slot = _inventory_ui->handleInput();
        if (backpack_slot != -1)
        {
            _player->equipItemFromBackpack(backpack_slot);
            return true;
        }

        const auto equipment_slot = _inventory_ui->getClickedEquipmentSlot();
        if (equipment_slot != Item::EquipmentSlot::None)
            _player->unequipItem(equipment_slot);

        return handleContainerPanelInput();
    }

    bool UIHandler::handleContainerPanelInput()
    {
        if (!_current_container)
            return false;

        auto* container_inventory = _current_container->getInventory();
        if (!container_inventory)
        {
            closeContainer();
            return true;
        }

        const int container_slot = _chest_ui->handleInput();
        if (container_slot == -1)
            return false;

        return pickUpContainerItem(*container_inventory, container_slot);
    }

    bool UIHandler::pickUpContainerItem(Item::Backpack& container_inventory, const int container_slot)
    {
        const auto item = container_inventory.getItem(container_slot);
        if (!item)
            return false;

        if (item->getId() == BABA_YAGA_BOOK_ITEM_ID)
        {
            if (_player->unlockFireballAbility())
                showNotification("Ksiega Baby Jagi rozsypala sie w popiol.", 3.0f);
            else
                showNotification("Znasz juz sekret tej ksiegi.", 2.5f);

            container_inventory.removeItem(container_slot);
            if (_quest_manager)
                _quest_manager->notifyItemCollected(item->getId());
            closeContainerIfEmpty(container_inventory);
            return true;
        }

        if (item->isFood())
        {
            _player->addFood(1);
            container_inventory.removeItem(container_slot);
            if (_quest_manager)
                _quest_manager->notifyItemCollected(item->getId());
            showNotification("Dodano jedzenie: " + item->getName(), 2.5f);
            closeContainerIfEmpty(container_inventory);
            return true;
        }

        if (!_player->getBackpack().addItem(item))
            return false;

        container_inventory.removeItem(container_slot);
        if (_quest_manager)
            _quest_manager->notifyItemCollected(item->getId());
        closeContainerIfEmpty(container_inventory);
        return true;
    }

    void UIHandler::closeContainerIfEmpty(Item::Backpack& container_inventory)
    {
        if (container_inventory.getRemainingCapacity() != container_inventory.getCapacity())
            return;

        showNotification("Ta skrzynia jest pusta", 3.0f);
        closeContainer();
    }
} // namespace Nawia::UI
