#include "Backpack.h"

namespace Nawia::Item {

    Backpack::Backpack(const int capacity) : _capacity(capacity) 
	{
        _items.resize(capacity, nullptr);
    }

    bool Backpack::addItem(const std::shared_ptr<Item>& item)
	{
        for (int i = 0; i < _capacity; ++i) 
        {
            if (_items[i] == nullptr) 
            {
                _items[i] = item;
                return true;
            }
        }
       return false;
    }

    void Backpack::removeItem(const int index) 
	{
        if (index >= 0 && index < _capacity)
            _items[index] = nullptr;
    }

    std::shared_ptr<Item> Backpack::getItem(const int index) const
	{
        if (index >= 0 && index < _capacity)
            return _items[index];

        return nullptr;
    }

    int Backpack::getRemainingCapacity() const 
	{
        int free_slots = 0;
        for (const auto& item : _items) 
        {
            if (item == nullptr)
                free_slots++;
        }
        return free_slots;
    }

} // namespace Nawia::Item