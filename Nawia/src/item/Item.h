#pragma once
#include <string>
#include <raylib.h>
#include <memory>
#include <Stats.h>
namespace Nawia::Item {

    // item type
    enum class EquipmentSlot {
        None,
        Head,
        Neck,
        Chest,
        Legs,
        Feet,
        Weapon,
        OffHand,
        Ring
    };

    class Item {
    public:
        Item(int id, std::string name, std::string description, EquipmentSlot type, const std::shared_ptr<Texture2D>& icon);
        virtual ~Item() = default;

        [[nodiscard]] int getId() const { return _id; }
        [[nodiscard]] const std::string& getName() const { return _name; }
        [[nodiscard]] const std::string& getDescription() const { return _description; }
        [[nodiscard]] EquipmentSlot getSlot() const { return _slot; }
        [[nodiscard]] Texture2D getIcon() const {
            if (_icon) return *_icon;
            return Texture2D{};
        }
        [[nodiscard]] const Entity::Stats& getStats() const { return _stats; }

        virtual bool use() { return false; }

        [[nodiscard]] virtual std::shared_ptr<Item> clone() const {
            return std::make_shared<Item>(*this);
        }

    protected:
        int _id;
        std::string _name;
        std::string _description;
        EquipmentSlot _slot;
        std::shared_ptr<Texture2D> _icon;
        Entity::Stats _stats;
    };

}