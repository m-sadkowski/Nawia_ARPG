#include "Loottable.h"
#include <fstream>
#include <iostream>
#include <string>
#include "json.hpp"
#include "Logger.h"
#include "ItemDatabase.h"

namespace Nawia::Item {

    LOOTTABLE_TYPE stringToLootType(const std::string& type_str) {
        if (type_str == "CAT") return LOOTTABLE_TYPE::CAT;
        if (type_str == "CHEST_NOOB") return LOOTTABLE_TYPE::CHEST_NOOB;
        if (type_str == "CHEST_BAD") return LOOTTABLE_TYPE::CHEST_BAD;
        if (type_str == "CHEST_GOOD") return LOOTTABLE_TYPE::CHEST_GOOD;

        // if not found use the worst
        return LOOTTABLE_TYPE::CHEST_NOOB;
    }

    bool Loottable::loadLootTables(const std::string& filename, ItemDatabase& item_db) 
	{
        std::ifstream file(filename);
        if (!file.is_open())
        {
            Core::Logger::errorLog("Could not open loot table file: " + filename);
            return false;
        }

        try 
        {
            json json_data;
            file >> json_data;

            if (!json_data.is_array()) 
            {
                Core::Logger::errorLog("Loot Tables - Wrong JSON format.");
                return false;
            }

            for (const auto& entry : json_data) 
            {
                std::string type_str = entry.value("type", "UNKNOWN");
                LOOTTABLE_TYPE type = stringToLootType(type_str);

                _loot_tables[type].clear();

                if (entry.contains("loot") && entry["loot"].is_object()) 
                {
                    for (auto& [idStr, chance_json] : entry["loot"].items()) {
                        const int item_id = std::stoi(idStr);
                        const float chance = chance_json;

                        if (const auto item_ptr = item_db.createItem(item_id)) 
                        {
                            LootEntry loot_entry;
                            loot_entry._item = item_ptr;
                            loot_entry._chance = chance;

                            _loot_tables[type].push_back(loot_entry);
                        }
                        else 
                        {
                            Core::Logger::errorLog("Loot table references unknown item ID: " + idStr);
                        }
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            Core::Logger::errorLog("Error parsing loot table: " + std::string(e.what()));
            return false;
        }

        return true;
    }

    std::vector<LootEntry> Loottable::getLootTable(const LOOTTABLE_TYPE loot_table) 
	{
        if (_loot_tables.find(loot_table) != _loot_tables.end())
            return _loot_tables[loot_table];

        return {};
    }

}