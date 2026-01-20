#pragma once
#include <raylib.h>
#include <memory>
#include <map>
#include <Player.h>
#include <GlobalScaling.h>

namespace Nawia::Core {
    class ResourceManager;
}

namespace Nawia::UI {

    class InventoryUI {
    public:
        InventoryUI();
        ~InventoryUI() = default;

        void loadResources(Core::ResourceManager& resourceManager);

        void render(const Font& font, const Entity::Player& player) const;

        // handle click on slot, -1 if none
        int handleInput() const;

        Item::EquipmentSlot getClickedEquipmentSlot() const;

    private:
        static constexpr float FONT_SIZE = 20.0f;
        static constexpr float SLOT_SIZE = 50.0f;
        static constexpr float PADDING = 10.0f;
        static constexpr float SLOT_PADDING = 4.0f;
        static constexpr float SLOT_PLACEHOLDER_PADDING = 8.0f;
        static constexpr int COLS = 5;
        static constexpr int ROWS = 4;

        static constexpr float INV_START_X = 20.0f;
        static constexpr float INV_START_Y = 20.0f;
        static constexpr float INV_WIDTH = 500.0f;
        static constexpr float INV_HEIGHT = 400.0f;
        static constexpr float TEXT_PADDING_LEFT = 20.0f;
        static constexpr float TEXT_PADDING_TOP = 10.0f;
        static constexpr float EQ_WIDTH = 220.0f;
        static constexpr float EQ_START_TOP = 50.0f;
        static constexpr float BP_START_TOP = 50.0f;
        static constexpr float GOLD_PADDING_BOTTOM = 30.0f;

        std::map<Item::EquipmentSlot, std::shared_ptr<Texture2D>> _placeholders;

        void drawSlot(int index, float x, float y, bool isHovered, const std::shared_ptr<Item::Item>& item) const;
        void drawSpecificSlot(Item::EquipmentSlot slotType, float x, float y, const Entity::Player& player, Vector2 mousePos) const;
    };

}