#include "Loottable.h"
#include <fstream>
#include <iostream>
#include <string>
#include "json.hpp"
#include "Logger.h"
#include "ItemDatabase.h"

namespace Nawia::Item {

    LOOTTABLE_TYPE stringToLootType(const std::string& typeStr) {
        if (typeStr == "CHEST_NOOB") return LOOTTABLE_TYPE::CHEST_NOOB;
        if (typeStr == "CHEST_BAD") return LOOTTABLE_TYPE::CHEST_BAD;
        if (typeStr == "CHEST_GOOD") return LOOTTABLE_TYPE::CHEST_GOOD;

        // if not found use the worst
        return LOOTTABLE_TYPE::CHEST_NOOB;
    }

    bool Loottable::loadLoottables(const std::string& filename, ItemDatabase& itemDb) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            Core::Logger::errorLog("Could not open loot table file: " + filename);
            return false;
        }

        try {
            json jsonData;
            file >> jsonData;

            if (!jsonData.is_array()) {
                Core::Logger::errorLog("Loottables - Wrong JSON format.");
                return false;
            }

            for (const auto& entry : jsonData) {
                std::string typeStr = entry.value("type", "UNKNOWN");
                LOOTTABLE_TYPE type = stringToLootType(typeStr);

                _loottables[type].clear();

                if (entry.contains("loot") && entry["loot"].is_object()) {
                    for (auto& [idStr, chanceJson] : entry["loot"].items()) {
                        int itemId = std::stoi(idStr);
                        float chance = (float)chanceJson;

                        auto itemPtr = itemDb.createItem(itemId);

                        if (itemPtr) {
                            LootEntry lootEntry;
                            lootEntry._item = itemPtr;
                            lootEntry._chance = chance;

                            _loottables[type].push_back(lootEntry);
                        }
                        else {
                            Core::Logger::errorLog("Loot table references unknown item ID: " + idStr);
                        }
                    }
                }
            }
        }
        catch (const std::exception& e) {
            Core::Logger::errorLog("Error parsing loot table: " + std::string(e.what()));
            return false;
        }

        return true;
    }

    std::vector<LootEntry> Loottable::getLoottable(LOOTTABLE_TYPE loottable) {
        if (_loottables.find(loottable) != _loottables.end()) {
            return _loottables[loottable];
        }
        return {};
    }

}