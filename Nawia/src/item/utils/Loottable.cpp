#include "Loottable.h"

#include <ItemDatabase.h>
#include <Logger.h>

#include <json.hpp>

#include <exception>
#include <fstream>
#include <string>

using json = nlohmann::json;

namespace Nawia::Item {

    LOOTTABLE_TYPE parseLoottableType(const std::string& type_name, const LOOTTABLE_TYPE default_type) {
        if (type_name == "CAT") return LOOTTABLE_TYPE::CAT;
        if (type_name == "CHEST_NOOB") return LOOTTABLE_TYPE::CHEST_NOOB;
        if (type_name == "CHEST_BAD") return LOOTTABLE_TYPE::CHEST_BAD;
        if (type_name == "CHEST_GOOD") return LOOTTABLE_TYPE::CHEST_GOOD;
        return default_type;
    }

    bool Loottable::loadLootTables(const std::string& filename, ItemDatabase& item_database) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            Core::Logger::errorLog("Loottable: nie mozna otworzyc pliku: " + filename);
            return false;
        }

        json json_data;
        try {
            file >> json_data;
        } catch (const json::parse_error& error) {
            Core::Logger::errorLog("Loottable: blad parsowania JSON: " + std::string(error.what()));
            return false;
        }

        if (!json_data.is_array()) {
            Core::Logger::errorLog("Loottable: niepoprawny format JSON.");
            return false;
        }

        _loot_tables.clear();

        for (const auto& table_entry : json_data) {
            const std::string type_name = table_entry.value("type", "UNKNOWN");
            const LOOTTABLE_TYPE type = parseLoottableType(type_name);
            auto& loot_entries = _loot_tables[type];
            loot_entries.clear();

            if (!table_entry.contains("loot") || !table_entry["loot"].is_object())
                continue;

            for (const auto& [item_id_text, chance_json] : table_entry["loot"].items()) {
                int item_id = 0;
                try {
                    item_id = std::stoi(item_id_text);
                } catch (const std::exception&) {
                    Core::Logger::errorLog("Loottable: niepoprawne ID przedmiotu: " + item_id_text);
                    continue;
                }

                float chance = 0.0f;
                try {
                    chance = chance_json.get<float>();
                } catch (const json::type_error& error) {
                    Core::Logger::errorLog("Loottable: niepoprawna szansa dla ID " + item_id_text + ": " + std::string(error.what()));
                    continue;
                }

                if (const auto item_template = item_database.createItem(item_id)) {
                    loot_entries.push_back({item_template, chance});
                } else {
                    Core::Logger::errorLog("Loottable: tabela odwoluje sie do nieznanego ID przedmiotu: " + item_id_text);
                }
            }
        }

        return true;
    }

    std::vector<LootEntry> Loottable::getLootTable(const LOOTTABLE_TYPE loot_table) {
        const auto table_it = _loot_tables.find(loot_table);
        if (table_it != _loot_tables.end())
            return table_it->second;

        return {};
    }

} // namespace Nawia::Item
