#pragma once

#include <Item.h>

#include <map>
#include <memory>
#include <string>

namespace Nawia::Core { class ResourceManager; }

namespace Nawia::Item {

    /**
     * @class ItemDatabase
     * @brief Wczytuje template'y przedmiotow i tworzy ich kopie po ID.
     */
    class ItemDatabase {
    public:
        /**
         * @brief Laduje baze przedmiotow z pliku JSON.
         */
        void loadDatabase(const std::string& filepath, Core::ResourceManager& resource_manager);

        /**
         * @brief Tworzy kopie przedmiotu z template'u albo nullptr.
         */
        std::shared_ptr<Item> createItem(int id);

        /**
         * @brief Zwraca template przedmiotu albo nullptr.
         */
        std::shared_ptr<Item> getItemTemplate(int id);

    private:
        std::map<int, std::shared_ptr<Item>> _templates;

        [[nodiscard]] EquipmentSlot stringToSlot(const std::string& slot_name) const;
    };

} // namespace Nawia::Item
