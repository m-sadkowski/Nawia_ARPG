#include "ItemDatabase.h"

#include <Boots.h>
#include <Chestplate.h>
#include <Equipment.h>
#include <Head.h>
#include <JsonUtils.h>
#include <Legs.h>
#include <Logger.h>
#include <Necklace.h>
#include <Offhand.h>
#include <ResourceManager.h>
#include <Ring.h>
#include <Weapon.h>

#include <json.hpp>

#include <set>

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

        int readDefenseStat(const json& entry) {
            if (!entry.contains("stats"))
                return 0;

            return entry["stats"].value("defense", 0);
        }

        void smoothItemIcon(const std::shared_ptr<Texture2D>& texture) {
            static std::set<unsigned int> smoothed_textures;

            if (!texture || texture->id <= 0)
                return;

            if (smoothed_textures.insert(texture->id).second) {
                GenTextureMipmaps(texture.get());
                SetTextureFilter(*texture, TEXTURE_FILTER_TRILINEAR);
            }
        }

    }

    void ItemDatabase::loadDatabase(const std::string& filepath, Core::ResourceManager& resource_manager) {
        const json data = Core::JsonUtils::loadDocument(filepath, "ItemDatabase");
        if (!data.is_array()) {
            Core::Logger::errorLog("ItemDatabase: niepoprawny format JSON: " + filepath);
            return;
        }

        _templates.clear();

        for (const auto& entry : data) {
            const int id = entry.value("id", 0);
            const std::string name = entry.value("name", "");
            const std::string description = entry.value("description", "");
            const std::string slot_name = entry.value("slot", "");
            const std::string texture_path = entry.value("texture", "");
			const std::string model_path = entry.value("model_path", "");
			if (!model_path.empty())
				resource_manager.getModel(model_path);

            const auto icon = resource_manager.getTexture(texture_path);
            if (!icon) {
                Core::Logger::errorLog("ItemDatabase: pominieto przedmiot bez tekstury ID " + std::to_string(id));
                continue;
            }
            smoothItemIcon(icon);

            const EquipmentSlot slot = Equipment::slotFromString(slot_name);
            std::shared_ptr<Item> item_template;

            if (slot == EquipmentSlot::Weapon) {
                item_template = std::make_shared<Weapon>(id, name, description, slot, icon, model_path, readIntStat(entry, "damage"));
            } else if (slot == EquipmentSlot::OffHand) {
                item_template = std::make_shared<Offhand>(
                    id,
                    name,
                    description,
                    slot,
                    icon,
					model_path,
					readIntStat(entry, "damage"),
                    readDefenseStat(entry)
                );
            } else if (slot == EquipmentSlot::Head) {
				item_template = std::make_shared<Head>(id, name, description, slot, icon, model_path,
				                                       readDefenseStat(entry));
            } else if (slot == EquipmentSlot::Neck) {
				item_template = std::make_shared<Necklace>(id, name, description, slot, icon, model_path,
				                                           readIntStat(entry, "intelligence"));
            } else if (slot == EquipmentSlot::Chest) {
				item_template = std::make_shared<Chestplate>(id, name, description, slot, icon, model_path,
				                                             readDefenseStat(entry));
            } else if (slot == EquipmentSlot::Legs) {
				item_template = std::make_shared<Legs>(id, name, description, slot, icon, model_path,
				                                       readDefenseStat(entry));
            } else if (slot == EquipmentSlot::Feet) {
                item_template = std::make_shared<Boots>(
                    id,
                    name,
                    description,
                    slot, icon, model_path,
                    readDefenseStat(entry),
                    readFloatStat(entry, "movement_speed")
                );
            } else if (slot == EquipmentSlot::Ring) {
				item_template = std::make_shared<Ring>(id, name, description, slot, icon, model_path,
				                                       readIntStat(entry, "intelligence"));
            } else {
				item_template = std::make_shared<Item>(id, name, description, slot, icon, model_path);
            }

			item_template->setFoodValue(entry.value("food_value", 0));
			Entity::Stats additional_stats;
			additional_stats.max_hp = readIntStat(entry, "max_hp");
			item_template->addStats(additional_stats);
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

} // namespace Nawia::Item
