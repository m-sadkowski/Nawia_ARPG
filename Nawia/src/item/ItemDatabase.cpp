#include "ItemDatabase.h"
#include "Weapon.h"
#include "Offhand.h"
#include "Head.h"
#include "Necklace.h"
#include "Chestplate.h"
#include "Legs.h"
#include "Boots.h"
#include "Ring.h"
#include <iostream>
#include "Logger.h"

namespace Nawia::Item {

    void ItemDatabase::loadDatabase(const std::string& filepath, Core::ResourceManager& resMgr) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Nie mozna otworzyc bazy przedmiotow: " << filepath << std::endl;
            return;
        }

        json data = json::parse(file);

        for (const auto& entry : data) {
            int id = entry["id"];
            std::string name = entry["name"];
            std::string desc = entry["description"];
            std::string slotStr = entry["slot"];
            std::string texPath = entry["texture"];

            auto icon = resMgr.getTexture("../" + texPath);
            if (!icon) {
                std::cerr << "error" << std::endl;
                continue;
            }

            EquipmentSlot slot = stringToSlot(slotStr);

            std::shared_ptr<Item> newItem = nullptr;

            if (slot == EquipmentSlot::Weapon) {
                int dmg = entry["stats"]["damage"];
                newItem = std::make_shared<Weapon>(id, name, desc, slot, icon, dmg);
            }
            else if (slot == EquipmentSlot::OffHand) {
                int dmg = entry["stats"]["damage"];
                int def = entry["stats"]["defense"];
                newItem = std::make_shared<Offhand>(id, name, desc, slot, icon, dmg, def);
            }
            else if (slot == EquipmentSlot::Head) {
                int armor = entry["stats"]["defense"];
                newItem = std::make_shared<Head>(id, name, desc, slot, icon, armor);
            }
            else if (slot == EquipmentSlot::Neck) {
                int intelligence = entry["stats"]["intelligence"];
                newItem = std::make_shared<Necklace>(id, name, desc, slot, icon, intelligence);
            }
            else if (slot == EquipmentSlot::Chest) {
                int armor = entry["stats"]["defense"];
                newItem = std::make_shared<Chestplate>(id, name, desc, slot, icon, armor);
            }
            else if (slot == EquipmentSlot::Legs) {
                int armor = entry["stats"]["defense"];
                newItem = std::make_shared<Legs>(id, name, desc, slot, icon, armor);
            }
            else if (slot == EquipmentSlot::Feet) {
                int armor = entry["stats"]["defense"];
                newItem = std::make_shared<Boots>(id, name, desc, slot, icon, armor);
            }
            else if (slot == EquipmentSlot::Ring) {
                int intelligence = entry["stats"]["intelligence"];
                newItem = std::make_shared<Ring>(id, name, desc, slot, icon, intelligence);
            }
            else {
                newItem = std::make_shared<Item>(id, name, desc, slot, icon);
            }

            // save template
            if (newItem) {
                _templates[id] = newItem;
                std::cout << "Zaladowano przedmiot ID " << id << ": " << name << std::endl;
            }
        }
    }

    std::shared_ptr<Item> ItemDatabase::createItem(int id) {
        if (_templates.find(id) != _templates.end()) {
            return _templates[id]->clone();
        }
        return nullptr;
    }

    EquipmentSlot ItemDatabase::stringToSlot(const std::string& str) {
        if (str == "Head") return EquipmentSlot::Head;
        if (str == "Chest") return EquipmentSlot::Chest;
        if (str == "Weapon") return EquipmentSlot::Weapon;
        return EquipmentSlot::None;
    }
}