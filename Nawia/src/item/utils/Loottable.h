#pragma once

#include <Item.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Nawia::Item {

    class ItemDatabase;

    /**
     * @enum LOOTTABLE_TYPE
     * @brief Typ tabeli lootu uzywanej przez skrzynie i NPC.
     */
    enum class LOOTTABLE_TYPE {
        CAT,
        CHEST_NOOB,
        CHEST_BAD,
        CHEST_GOOD
    };

    /**
     * @struct LootEntry
     * @brief Jeden wpis lootu z template'em przedmiotu i szansa wylosowania.
     */
    struct LootEntry {
        std::shared_ptr<Item> _item;
        float _chance = 0.0f;
    };

    /**
     * @class Loottable
     * @brief Wczytuje tabele lootu i przechowuje template'y przedmiotow.
     */
    class Loottable {
    public:
        /**
         * @brief Wczytuje tabele lootu z JSON.
         */
        bool loadLootTables(const std::string& filename, ItemDatabase& item_database);

        /**
         * @brief Zwraca wpisy dla wybranej tabeli.
         */
        std::vector<LootEntry> getLootTable(LOOTTABLE_TYPE loot_table);

    private:
        std::map<LOOTTABLE_TYPE, std::vector<LootEntry>> _loot_tables;
    };

} // namespace Nawia::Item
