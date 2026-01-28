#pragma once
#include "Item.h"
#include <vector>
#include <memory>

namespace Nawia::Item {

    class Backpack {
    public:
        Backpack(int capacity);

        // return false when no more space
        bool addItem(const std::shared_ptr<Item>& item);

        // remove item at index
        void removeItem(int index);

        [[nodiscard]] std::shared_ptr<Item> getItem(int index) const;
        [[nodiscard]] const std::vector<std::shared_ptr<Item>>& getItems() const { return _items; }
        [[nodiscard]] int getCapacity() const { return _capacity; }
        [[nodiscard]] int getRemainingCapacity() const;

    private:
        int _capacity;
        std::vector<std::shared_ptr<Item>> _items;
    };

} // namespace Nawia::Item