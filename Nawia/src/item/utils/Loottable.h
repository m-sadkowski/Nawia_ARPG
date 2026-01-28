#pragma once
#include <map>
#include <vector>
#include <string>
#include <memory>
#include "Item.h"

namespace Nawia::Item {

	class ItemDatabase;

	enum class LOOTTABLE_TYPE 
	{
		CAT,
		CHEST_NOOB,
		CHEST_BAD,
		CHEST_GOOD
	};

	struct LootEntry 
	{
		std::shared_ptr<Item> _item;
		float _chance;
	};

	class Loottable {
	public:
		bool loadLootTables(const std::string& filename, ItemDatabase& item_db);

		std::vector<LootEntry> getLootTable(LOOTTABLE_TYPE loot_table);
	private:
		// stores item TEMPLATES, use item->clone to create a new item
		std::map<LOOTTABLE_TYPE, std::vector<LootEntry>> _loot_tables;
	};
}