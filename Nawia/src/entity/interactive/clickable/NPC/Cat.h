#pragma once

#include <Dialogue.h>
#include <InteractiveClickable.h>

#include <memory>
#include <string>

namespace Nawia::Item {
	class Backpack;
	class Item;
	class Loottable;
	enum class LOOTTABLE_TYPE;
}

namespace Nawia::Entity {

	/**
	 * @class Cat
	 * @brief Klikalny NPC obsługujący prostą wymianę questową z graczem.
	 */
	class Cat : public InteractiveClickable {
	public:
		/** @brief Tworzy kota w podanym punkcie świata. */
		Cat(const std::string& name, float x, float y, const std::shared_ptr<Texture2D>& texture);
		~Cat() override;

		/**
		 * @brief Losuje startowy ekwipunek kota z tabeli łupów.
		 */
		void initializeInventory(Item::Loottable& lootable, Item::LOOTTABLE_TYPE lootable_type) const;

		/** @brief Obsługuje oddanie ryby i zakończenie questu kota. */
		void onInteract(Entity& instigator) override;

		/** @brief Aktualizuje bazowy stan NPC. */
		void update(float delta_time) override;

		/** @brief Renderuje model kota. */
		void render(const Camera3D& camera) override;

		/** @brief Zwraca zasięg interakcji z kotem. */
		float getInteractionRange() override;

		/** @brief Udostępnia ekwipunek kota. */
		Item::Backpack* getInventory() override;

		/** @brief Dodaje przedmiot do ekwipunku kota. */
		void addItem(const std::shared_ptr<Item::Item>& item) const;

		/** @brief Zwraca drzewo dialogowe przypisane do kota. */
		[[nodiscard]] const Game::DialogueTree& getDialogueTree() const { return _dialogue_tree; }

		/** @brief Podmienia drzewo dialogowe kota. */
		void setDialogue(const Game::DialogueTree& dialogue) { _dialogue_tree = dialogue; }

		using InteractiveClickable::canInteract;

		/** @brief Blokuje interakcję po zakończeniu questu. */
		bool canInteract() const override {
			if (_quest_completed)
				return false;

			return InteractiveClickable::canInteract();
		}

	private:
		bool _is_open = false;
		bool _quest_completed = false;

		std::unique_ptr<Item::Backpack> _inventory;
		static constexpr int INVENTORY_SIZE = 5;
		Game::DialogueTree _dialogue_tree;
	};

} // namespace Nawia::Entity
