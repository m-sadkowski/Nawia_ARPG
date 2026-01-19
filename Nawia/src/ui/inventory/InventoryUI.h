#pragma once
#include <raylib.h>
#include <memory>
#include <Player.h>

namespace Nawia::UI {

    class InventoryUI {
    public:
        InventoryUI();
        ~InventoryUI() = default;

        void render(const Entity::Player& player) const;

        // handle click on slot, -1 if none
        int handleInput() const;

    private:
        static constexpr float SLOT_SIZE = 50.0f;
        static constexpr float PADDING = 10.0f;
        static constexpr int COLS = 5;
        static constexpr int ROWS = 4;

        Vector2 _position = { 100.0f, 100.0f };

        void drawSlot(int index, float x, float y, bool isHovered, const std::shared_ptr<Item::Item>& item) const;
    };

}