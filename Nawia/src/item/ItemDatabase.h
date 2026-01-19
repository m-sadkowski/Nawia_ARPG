#pragma once
#include <map>
#include <string>
#include <memory>
#include <vector>
#include <fstream>
#include "json.hpp"
#include "ResourceManager.h"
#include "Item.h"

using json = nlohmann::json;

namespace Nawia::Item {

    class ItemDatabase {
    public:
        void loadDatabase(const std::string& filepath, Core::ResourceManager& resMgr);

        std::shared_ptr<Item> createItem(int id);

    private:
        // id -> item object
        std::map<int, std::shared_ptr<Item>> _templates;

        // conversion to slot
        EquipmentSlot stringToSlot(const std::string& str);
    };
}