#pragma once

#include "EntityFactory.h"

#include <AssetPathUtils.h>
#include <Engine.h>
#include <Item.h>
#include <JsonUtils.h>
#include <Loottable.h>

#include <json.hpp>

#include <memory>
#include <string>

namespace Nawia::World::EntityFactoryDetail {

	using json = nlohmann::json;

	struct SpawnBasics {
		Vector2 position = {0.0f, 0.0f};
		int hp = 1;
		std::string name;
	};

	inline SpawnBasics readBasics(const json& data, const std::string& default_name, const int default_hp = 1) {
		return {
			{data.value("x", 0.0f), data.value("y", 0.0f)},
			data.value("hp", default_hp),
			data.value("name", default_name)
		};
	}

	inline Item::LOOTTABLE_TYPE parseLoottableType(
		const std::string& loottable_name,
		Item::LOOTTABLE_TYPE default_type
	) {
		if (loottable_name == "CHEST_BAD") return Item::LOOTTABLE_TYPE::CHEST_BAD;
		if (loottable_name == "CAT") return Item::LOOTTABLE_TYPE::CAT;
		if (loottable_name == "CHEST_NOOB") return Item::LOOTTABLE_TYPE::CHEST_NOOB;
		return default_type;
	}

	template <typename AddItem>
	void addItemsFromJson(const json& data, Core::Engine* engine, AddItem add_item) {
		if (!engine || !data.contains("items") || !data["items"].is_array())
			return;

		auto& item_database = engine->getItemDatabase();
		for (const auto& item_id : data["items"]) {
			if (!item_id.is_number_integer())
				continue;

			if (auto item = item_database.createItem(item_id.get<int>()))
				add_item(item);
		}
	}

	using CreatorFn = std::shared_ptr<Entity::Entity> (*)(const json&, const SpawnContext&);
	using NpcCreatorFn = std::shared_ptr<Entity::Entity> (*)(
		const json&,
		const SpawnContext&,
		const SpawnBasics&
	);

} // namespace Nawia::World::EntityFactoryDetail
