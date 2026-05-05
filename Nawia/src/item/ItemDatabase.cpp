#include "ItemDatabase.h"

#include <Boots.h>
#include <Chestplate.h>
#include <Head.h>
#include <Legs.h>
#include <Logger.h>
#include <Necklace.h>
#include <Offhand.h>
#include <ResourceManager.h>
#include <Ring.h>
#include <Weapon.h>

#include <json.hpp>

#include <fstream>

using json = nlohmann::json;

namespace Nawia::Item {

    namespace {

        int readIntStat(const json& entry, const char* name, const int fallback = 0) {
            if (!entry.contains("stats"))
                return fallback;

            return entry["stats"].value(name, fallback);
        }

        float readFloatStat(const json& entry, const char* name, const float fallback = 0.0f) {
            if (!entry.contains("stats"))
                return fallback;

            return entry["stats"].value(name, fallback);
        }

    }

    void ItemDatabase::loadDatabase(const std::string& filepath, Core::ResourceManager& resource_manager) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            Core::Logger::errorLog("ItemDatabase: nie mozna otworzyc bazy przedmiotow: " + filepath);
            return;
        }

        json data;
        try {
            file >> data;
        } catch (const json::parse_error& error) {
            Core::Logger::errorLog("ItemDatabase: blad parsowania JSON: " + std::string(error.what()));
            return;
        }

        _templates.clear();

        for (const auto& entry : data) {
            const int id = entry.value("id", 0);
            const std::string name = entry.value("name", "");
            const std::string description = entry.value("description", "");
            const std::string slot_name = entry.value("slot", "");
            const std::string texture_path = entry.value("texture", "");

            const auto icon = resource_manager.getTexture(texture_path);
            if (!icon) {
                Core::Logger::errorLog("ItemDatabase: pominieto przedmiot bez tekstury ID " + std::to_string(id));
                continue;
            }

            const EquipmentSlot slot = stringToSlot(slot_name);
            std::shared_ptr<Item> item_template;

            if (slot == EquipmentSlot::Weapon) {
                item_template = std::make_shared<Weapon>(id, name, description, slot, icon, readIntStat(entry, "damage"));
            } else if (slot == EquipmentSlot::OffHand) {
                item_template = std::make_shared<Offhand>(
                    id,
                    name,
                    description,
                    slot,
                    icon,
                    readIntStat(entry, "damage"),
                    readIntStat(entry, "defense")
                );
            } else if (slot == EquipmentSlot::Head) {
                item_template = std::make_shared<Head>(id, name, description, slot, icon, readIntStat(entry, "defense"));
            } else if (slot == EquipmentSlot::Neck) {
                item_template = std::make_shared<Necklace>(id, name, description, slot, icon, readIntStat(entry, "intelligence"));
            } else if (slot == EquipmentSlot::Chest) {
                item_template = std::make_shared<Chestplate>(id, name, description, slot, icon, readIntStat(entry, "defense"));
            } else if (slot == EquipmentSlot::Legs) {
                item_template = std::make_shared<Legs>(id, name, description, slot, icon, readIntStat(entry, "defense"));
            } else if (slot == EquipmentSlot::Feet) {
                item_template = std::make_shared<Boots>(
                    id,
                    name,
                    description,
                    slot,
                    icon,
                    readIntStat(entry, "defense"),
                    readFloatStat(entry, "movement_speed")
                );
            } else if (slot == EquipmentSlot::Ring) {
                item_template = std::make_shared<Ring>(id, name, description, slot, icon, readIntStat(entry, "intelligence"));
            } else {
                item_template = std::make_shared<Item>(id, name, description, slot, icon);
            }

            _templates[id] = item_template;
            Core::Logger::debugLog("Zaladowano przedmiot ID " + std::to_string(id) + ": " + name);
        }
    }

    std::shared_ptr<Item> ItemDatabase::createItem(const int id) {
        const auto template_it = _templates.find(id);
        if (template_it != _templates.end())
            return template_it->second->clone();

        return nullptr;
    }

    std::shared_ptr<Item> ItemDatabase::getItemTemplate(const int id) {
        const auto template_it = _templates.find(id);
        if (template_it != _templates.end())
            return template_it->second;

        return nullptr;
    }

    EquipmentSlot ItemDatabase::stringToSlot(const std::string& slot_name) const {
        if (slot_name == "Head") return EquipmentSlot::Head;
        if (slot_name == "Neck") return EquipmentSlot::Neck;
        if (slot_name == "Chest") return EquipmentSlot::Chest;
        if (slot_name == "Legs") return EquipmentSlot::Legs;
        if (slot_name == "Feet") return EquipmentSlot::Feet;
        if (slot_name == "Weapon") return EquipmentSlot::Weapon;
        if (slot_name == "OffHand") return EquipmentSlot::OffHand;
        if (slot_name == "Ring") return EquipmentSlot::Ring;
        return EquipmentSlot::None;
    }

} // namespace Nawia::Item
