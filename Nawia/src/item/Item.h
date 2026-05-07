#pragma once

#include <Stats.h>

#include <raylib.h>

#include <memory>
#include <string>

namespace Nawia::Item {

    /**
     * @enum EquipmentSlot
     * @brief Slot ekwipunku, do ktorego moze trafic przedmiot.
     */
    enum class EquipmentSlot {
        None,
        Head,
        Neck,
        Chest,
        Legs,
        Feet,
        Weapon,
        OffHand,
        Ring
    };

    /**
     * @class Item
     * @brief Bazowy przedmiot trzymany w plecaku, ekwipunku albo loottable.
     *
     * Ikona jest wspoldzielona przez `shared_ptr`, bo pochodzi z cache
     * ResourceManagera i moze byc uzywana przez wiele kopii przedmiotu.
     */
    class Item {
    public:
		Item(int id, std::string name, std::string description, EquipmentSlot type,
		     const std::shared_ptr<Texture2D>& icon, std::string model_path = "");
        virtual ~Item() = default;

        /** @brief Zwraca ID przedmiotu. */
        [[nodiscard]] int getId() const { return _id; }

        /** @brief Zwraca nazwe przedmiotu. */
        [[nodiscard]] const std::string& getName() const { return _name; }

        /** @brief Zwraca opis przedmiotu. */
        [[nodiscard]] const std::string& getDescription() const { return _description; }

        /** @brief Zwraca slot, do ktorego przedmiot moze zostac zalozony. */
        [[nodiscard]] EquipmentSlot getSlot() const { return _slot; }

        /** @brief Zwraca teksture ikony albo pusta teksture. */
        [[nodiscard]] Texture2D getIcon() const {
            if (_icon) return *_icon;
            return Texture2D{};
        }

        /** @brief Zwraca statystyki dawane przez przedmiot. */
        [[nodiscard]] const Entity::Stats& getStats() const { return _stats; }

		[[nodiscard]] const std::string& getModelPath() const { return _model_path; }

        /** @brief Uzywa przedmiotu, jesli konkretny typ to wspiera. */
        virtual bool use() { return false; }

        /** @brief Tworzy kopie przedmiotu z template'u. */
        [[nodiscard]] virtual std::shared_ptr<Item> clone() const {
            return std::make_shared<Item>(*this);
        }

    protected:
        int _id;
        std::string _name;
        std::string _description;
        EquipmentSlot _slot;
        std::shared_ptr<Texture2D> _icon;
        Entity::Stats _stats;
		std::string _model_path;
    };

} // namespace Nawia::Item
