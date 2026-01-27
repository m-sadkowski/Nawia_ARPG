#pragma once
#include "InteractiveClickable.h"
#include "Entity.h"
#include "Backpack.h"
#include "Loottable.h"
#include "Dialogue.h"

namespace Nawia::Entity {

    class Cat : public InteractiveClickable {
    public:
        Cat(const std::string& name, float x, float y, const std::shared_ptr<Texture2D>& texture);

        void initializeInventory(Item::Loottable& lootable, Item::LOOTTABLE_TYPE lootable_type) const;

        void onInteract(Entity& instigator) override;

        void update(float delta_time) override;
        void render(float offset_x, float offset_y) override;
        float getInteractionRange() override;

        Item::Backpack* getInventory() override { return _inventory.get(); }
        void addItem(const std::shared_ptr<Item::Item>& item) const {
            _inventory->addItem(item);
        }

        [[nodiscard]] const Game::DialogueTree& getDialogueTree() const { return _dialogueTree; }
        void setDialogue(const Game::DialogueTree& dialogue) { _dialogueTree = dialogue; }
    private:
        bool _isOpen = false;
        std::unique_ptr<Item::Backpack> _inventory;
        static constexpr int _inv_size = 1;
        Game::DialogueTree _dialogueTree;
    };

}