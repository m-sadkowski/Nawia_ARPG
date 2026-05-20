#pragma once

#include <Item.h>

#include <memory>
#include <vector>

namespace Nawia::Item {

    /**
     * @class Backpack
     * @brief Przechowuje przedmioty gracza albo kontenera w slotach.
     *
     * Przedmioty sa trzymane jako `shared_ptr`, bo ten sam obiekt moze byc
     * chwilowo widziany przez UI podczas przenoszenia miedzy slotami.
     */
    class Backpack {
    public:
        explicit Backpack(int capacity);

        /** @brief Dodaje przedmiot do pierwszego wolnego slotu. */
        bool addItem(const std::shared_ptr<Item>& item);

        /** @brief Usuwa przedmiot z podanego slotu. */
        void removeItem(int index);

        /** @brief Ustawia zawartosc konkretnego slotu. */
        bool setItem(int index, const std::shared_ptr<Item>& item);

        /** @brief Oproznia wszystkie sloty. */
        void clear();

        /** @brief Zwraca przedmiot z indeksu albo nullptr. */
        [[nodiscard]] std::shared_ptr<Item> getItem(int index) const;

        /** @brief Zwraca wszystkie sloty plecaka. */
        [[nodiscard]] const std::vector<std::shared_ptr<Item>>& getItems() const { return _items; }

        /** @brief Zwraca liczbe slotow plecaka. */
        [[nodiscard]] int getCapacity() const { return _capacity; }

        /** @brief Zwraca liczbe pustych slotow. */
        [[nodiscard]] int getRemainingCapacity() const;

    private:
        int _capacity;
        std::vector<std::shared_ptr<Item>> _items;
    };

} // namespace Nawia::Item
