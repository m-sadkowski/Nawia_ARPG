#pragma once
#include <map>
#include <vector>
#include <string>
#include <memory>
#include "Item.h"

namespace Nawia::Item {

	class ItemDatabase;

	enum class LOOTTABLE_TYPE {
		CAT,
		CHEST_NOOB,
		CHEST_BAD,
		CHEST_GOOD
	};

	struct LootEntry {
		std::shared_ptr<Item> _item;
		float _chance;
	};

	class Loottable {
	public:
		bool loadLoottables(const std::string& filename, ItemDatabase& itemDb);

		std::vector<LootEntry> getLoottable(LOOTTABLE_TYPE loottable);
	private:
		// stores item TEMPLATES, use item->clone to create a new item
		std::map<LOOTTABLE_TYPE, std::vector<LootEntry>> _loottables;
	};
}