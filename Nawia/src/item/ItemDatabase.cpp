#include "ItemDatabase.h"
#include "Weapon.h"
#include "Offhand.h"
#include "Head.h"
#include "Necklace.h"
#include "Chestplate.h"
#include "Legs.h"
#include "Boots.h"
#include "Ring.h"
#include "Logger.h"

namespace Nawia::Item {

    void ItemDatabase::loadDatabase(const std::string& filepath, Core::ResourceManager& res_mgr) 
	{
        std::ifstream file(filepath);
        if (!file.is_open()) 
        {
            Core::Logger::errorLog("Nie mozna otworzyc bazy przedmiotow: " + filepath);
            return;
        }

        json data = json::parse(file);

        for (const auto& entry : data) 
        {
            int id = entry["id"];
            std::string name = entry["name"];
            std::string desc = entry["description"];
            std::string slot_str = entry["slot"];
            std::string tex_path = entry["texture"];

            const auto icon = res_mgr.getTexture("../" + tex_path);
            if (!icon)
                continue;

            EquipmentSlot slot = stringToSlot(slot_str);

            std::shared_ptr<Item> new_item = nullptr;

            if (slot == EquipmentSlot::Weapon) 
            {
                int dmg = entry["stats"]["damage"];
                new_item = std::make_shared<Weapon>(id, name, desc, slot, icon, dmg);
            }
            else if (slot == EquipmentSlot::OffHand) 
            {
                int dmg = entry["stats"]["damage"];
                int def = entry["stats"]["defense"];
                new_item = std::make_shared<Offhand>(id, name, desc, slot, icon, dmg, def);
            }
            else if (slot == EquipmentSlot::Head) 
            {
                int armor = entry["stats"]["defense"];
                new_item = std::make_shared<Head>(id, name, desc, slot, icon, armor);
            }
            else if (slot == EquipmentSlot::Neck) 
            {
                int intelligence = entry["stats"]["intelligence"];
                new_item = std::make_shared<Necklace>(id, name, desc, slot, icon, intelligence);
            }
            else if (slot == EquipmentSlot::Chest) 
            {
                int armor = entry["stats"]["defense"];
                new_item = std::make_shared<Chestplate>(id, name, desc, slot, icon, armor);
            }
            else if (slot == EquipmentSlot::Legs) 
            {
                int armor = entry["stats"]["defense"];
                new_item = std::make_shared<Legs>(id, name, desc, slot, icon, armor);
            }
            else if (slot == EquipmentSlot::Feet) 
            {
                int armor = entry["stats"].value("defense", 0);
                float move_speed = entry["stats"].value("movement_speed", 0.0f);
                new_item = std::make_shared<Boots>(id, name, desc, slot, icon, armor, move_speed);
            }
            else if (slot == EquipmentSlot::Ring) 
            {
                int intelligence = entry["stats"]["intelligence"];
                new_item = std::make_shared<Ring>(id, name, desc, slot, icon, intelligence);
            }
            else 
            	{
                new_item = std::make_shared<Item>(id, name, desc, slot, icon);
            }

            // save template
            if (new_item) {
                _templates[id] = new_item;
                Core::Logger::debugLog("Zaladowano przedmiot ID " + std::to_string(id) + ": " + name);
            }
        }
    }

    std::shared_ptr<Item> ItemDatabase::createItem(const int id) 
	{
        if (_templates.find(id) != _templates.end())
            return _templates[id]->clone();

        return nullptr;
    }

    std::shared_ptr<Item> ItemDatabase::getItemTemplate(const int id) 
	{
        if (_templates.find(id) != _templates.end())
            return _templates[id];

        return nullptr;
    }

    EquipmentSlot ItemDatabase::stringToSlot(const std::string& str) 
	{
        if (str == "Head") return EquipmentSlot::Head;
        if (str == "Chest") return EquipmentSlot::Chest;
        if (str == "Weapon") return EquipmentSlot::Weapon;
        if (str == "Feet") return EquipmentSlot::Feet;
        return EquipmentSlot::None;
    }
}