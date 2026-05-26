#pragma once

#include <InteractiveClickable.h>

#include <memory>
#include <string>

namespace Nawia::Item {
	class Backpack;
	class Item;
	class ItemDatabase;
	class Loottable;
	enum class LOOTTABLE_TYPE;
}

namespace Nawia::Entity {

	/**
	 * @class Chest
	 * @brief Klikalna skrzynia z ekwipunkiem i opcjonalną blokadą na klucz.
	 */
	class Chest : public InteractiveClickable {
	public:
		/** @brief Tworzy skrzynię w podanym punkcie świata. */
		Chest(const std::string& name, float x, float y, const std::shared_ptr<Texture2D>& texture);
		~Chest() override;

		/**
		 * @brief Losuje startową zawartość skrzyni z podanej tabeli łupów.
		 */
		void initializeInventory(Item::Loottable& loot_table, Item::LOOTTABLE_TYPE loot_table_type);

		/** @brief Otwiera skrzynię, jeśli nie jest zablokowana. */
		void onInteract(Entity& instigator) override;

		/** @brief Aktualizuje bazowy stan skrzyni. */
		void update(float delta_time) override;

		/** @brief Renderuje skrzynię i diagnostykę encji. */
		void render(const Camera3D& camera) override;

		/** @brief Zwraca zasięg kliknięcia skrzyni. */
		float getInteractionRange() override;

		/** @brief Udostępnia ekwipunek, gdy skrzynia nie jest zablokowana. */
		Item::Backpack* getInventory() override;

		/** @brief Dodaje przedmiot do ekwipunku skrzyni. */
		void addItem(const std::shared_ptr<Item::Item>& item);

		/** @brief Ustawia blokadę skrzyni i identyfikator wymaganego klucza. */
		void setLocked(bool locked, int key_id);
		void setOpen(bool open) { _is_open = open; }
		[[nodiscard]] bool isOpen() const { return _is_open; }
		[[nodiscard]] bool isLocked() const { return _locked; }
		[[nodiscard]] int getKeyId() const { return _key_id; }

		[[nodiscard]] nlohmann::json serializeState() const override;
		void applyState(const nlohmann::json& state, Item::ItemDatabase* item_database = nullptr) override;

	private:
		bool _is_open = false;
		bool _locked = false;
		int _key_id = -1;

		std::unique_ptr<Item::Backpack> _inventory;
		static constexpr int INVENTORY_SIZE = 12;
	};

} // namespace Nawia::Entity
