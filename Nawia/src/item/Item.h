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
        Item(int id, std::string name, std::string description, EquipmentSlot type, std::shared_ptr<Texture2D> icon);
        virtual ~Item() = default;

        const int getId() const { return _id; }
        const std::string& getName() const { return _name; }
        const std::string& getDescription() const { return _description; }
        EquipmentSlot getSlot() const { return _slot; }
        Texture2D getIcon() const {
            if (_icon) return *_icon;
            return Texture2D{ 0 };
        }

        const Entity::Stats& getStats() const { return _stats; }

        virtual bool use() { return false; }

        virtual std::shared_ptr<Item> clone() const {
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