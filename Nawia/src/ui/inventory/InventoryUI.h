#pragma once

#include <GlobalScaling.h>
#include <Player.h>

#include <raylib.h>

#include <map>
#include <memory>

namespace Nawia::Core {
    class ResourceManager;
}

namespace Nawia::UI {

    /**
     * @class InventoryUI
     * @brief Rysuje ekwipunek, plecak i sloty wyposazenia gracza.
     */
    class InventoryUI {
    public:
        InventoryUI();
        ~InventoryUI() = default;

        static constexpr float INV_WIDTH = 724.0f;
        static constexpr float INV_HEIGHT = 543.0f;

        /** @brief Laduje grafiki zastepcze pustych slotow ekwipunku. */
        void loadResources(Core::ResourceManager& resource_manager);

        /** @brief Rysuje panel ekwipunku gracza i plecaka. */
        void render(const Font& font, const Entity::Player& player) const;

        /** @brief Zwraca klikniety slot plecaka albo -1, gdy klikniecie bylo poza plecakiem. */
        int handleInput() const;

        /** @brief Zwraca klikniety slot wyposazenia albo None. */
        Item::EquipmentSlot getClickedEquipmentSlot() const;

    private:
        static constexpr float FONT_SIZE = 20.0f;
        static constexpr float SLOT_SIZE = 52.0f;
        static constexpr float PADDING = 10.0f;
        static constexpr float SLOT_PADDING = 4.0f;
        static constexpr float SLOT_PLACEHOLDER_PADDING = 8.0f;
        static constexpr float SLOT_SPACING = 6.0f;
        static constexpr int BACKPACK_COLUMNS = 4;
        static constexpr int BACKPACK_ROWS = 5;
        static constexpr int BACKPACK_SLOT_COUNT = BACKPACK_COLUMNS * BACKPACK_ROWS;

        static constexpr float INV_START_X = 20.0f;
        static constexpr float INV_START_Y = 20.0f;
        static constexpr float TEXT_PADDING_LEFT = 20.0f;
        static constexpr float TEXT_PADDING_TOP = 10.0f;
        static constexpr float EQ_WIDTH = 220.0f;
        static constexpr float EQ_START_TOP = 50.0f;
        static constexpr float BP_START_TOP = 50.0f;
        static constexpr float GOLD_PADDING_BOTTOM = 30.0f;

        std::map<Item::EquipmentSlot, std::shared_ptr<Texture2D>> _placeholders;
        std::shared_ptr<Texture2D> _background;

        /** @brief Oblicza prostokat panelu ekwipunku. */
        [[nodiscard]] Rectangle getInventoryRect() const;

        /** @brief Oblicza prostokat slotu plecaka na podstawie indeksu. */
        [[nodiscard]] Rectangle getBackpackSlotRect(int index) const;

        /** @brief Oblicza prostokat slotu wyposazenia na podstawie typu slotu. */
        [[nodiscard]] Rectangle getEquipmentSlotRect(Item::EquipmentSlot slot_type) const;

        /** @brief Rysuje slot plecaka wraz z ikona przedmiotu. */
        void drawSlot(int index, Rectangle slot_rect, bool is_hovered, const std::shared_ptr<Item::Item>& item) const;

        /** @brief Rysuje konkretny slot wyposazenia, np. bron albo buty. */
        void drawSpecificSlot(Item::EquipmentSlot slot_type, const Entity::Player& player, Vector2 mouse_position) const;

        /** @brief Rysuje podpowiedz przedmiotu przy kursorze. */
        void drawTooltip(const Font& font, const std::shared_ptr<Item::Item>& item, float x, float y) const;
    };

} // namespace Nawia::UI
