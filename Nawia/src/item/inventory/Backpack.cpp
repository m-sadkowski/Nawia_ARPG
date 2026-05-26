#include "Backpack.h"

#include "ItemDatabase.h"

namespace Nawia::Item {

    Backpack::Backpack(const int capacity) : _capacity(capacity) {
        _items.resize(capacity, nullptr);
    }

    bool Backpack::addItem(const std::shared_ptr<Item>& item) {
        if (!item)
            return false;

        for (int i = 0; i < _capacity; ++i) {
            if (_items[i] == nullptr) {
                _items[i] = item;
                return true;
            }
        }
        return false;
    }

    void Backpack::removeItem(const int index) {
        if (index >= 0 && index < _capacity)
            _items[index] = nullptr;
    }

    bool Backpack::setItem(const int index, const std::shared_ptr<Item>& item) {
        if (index < 0 || index >= _capacity)
            return false;

        _items[index] = item;
        return true;
    }

    void Backpack::clear() {
        for (auto& item : _items)
            item = nullptr;
    }

    std::shared_ptr<Item> Backpack::getItem(const int index) const {
        if (index >= 0 && index < _capacity)
            return _items[index];

        return nullptr;
    }

    int Backpack::getRemainingCapacity() const {
        int free_slots = 0;
        for (const auto& item : _items) {
            if (item == nullptr)
                free_slots++;
        }
        return free_slots;
    }

    nlohmann::json Backpack::serialize() const {
        nlohmann::json result;
        result["capacity"] = _capacity;
        result["items"] = nlohmann::json::array();

        for (size_t i = 0; i < _items.size(); ++i) {
            if (!_items[i])
                continue;

            result["items"].push_back({
                {"slot", i},
                {"item_id", _items[i]->getId()}
            });
        }

        return result;
    }

    void Backpack::applyJson(const nlohmann::json& data, ItemDatabase& item_database) {
        clear();

        if (!data.contains("items") || !data["items"].is_array())
            return;

        for (const auto& item_state : data["items"]) {
            const int slot = item_state.value("slot", -1);
            const int item_id = item_state.value("item_id", 0);
            if (slot < 0 || item_id <= 0)
                continue;

            if (auto item = item_database.createItem(item_id))
                setItem(slot, item);
        }
    }

} // namespace Nawia::Item
