#include "ItemDatabase.h"
#include "Weapon.h"
#include "Chestplate.h"
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
            else if (slot == EquipmentSlot::Chest) {
                int armor = entry["stats"]["armor"];
                newItem = std::make_shared<Chestplate>(id, name, desc, slot, icon, armor);
            }
            // todo add other types

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