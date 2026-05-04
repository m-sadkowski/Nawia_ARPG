#include "Cat.h"

#include <Backpack.h>
#include <Collider.h>
#include <Engine.h>
#include <Loottable.h>
#include <Player.h>

namespace Nawia::Entity {

	namespace {
		constexpr int REQUIRED_FISH_ID = 6;
		constexpr int CAT_SWORD_ID = 7;
	}

	Cat::Cat(const std::string& name, const float x, const float y, const std::shared_ptr<Texture2D>& texture)
		: InteractiveClickable(name, x, y, texture, 1) // NPC ma techniczne 1 HP.
	{
		_type = EntityType::NPCStatic;
		setFaction(Faction::None);
		setScale(0.03f);
		loadModel("../assets/models/cat_bounce.glb", false);
		playAnimation("default");

		_inventory = std::make_unique<Item::Backpack>(INVENTORY_SIZE);
	}

	Cat::~Cat() = default;

	void Cat::initializeInventory(Item::Loottable& lootable, const Item::LOOTTABLE_TYPE lootable_type) const {
		const auto& drops = lootable.getLootTable(lootable_type);

		for (const auto& entry : drops) {
			if (!entry._item)
				continue;

			const float roll = static_cast<float>(GetRandomValue(0, 10000)) / 100.0f;

			if (roll <= entry._chance) {
				const std::shared_ptr<Item::Item> unique_item = entry._item->clone();
				addItem(unique_item);
			}
		}
	}

	void Cat::onInteract(Entity& instigator) {
		if (_quest_completed)
			return;

		if (auto* player = dynamic_cast<Player*>(&instigator)) {
			// Szukamy ryby wymaganej do zakończenia questu.
			int fish_index = -1;
			auto& backpack = player->getBackpack();
			const auto& items = backpack.getItems();

			for (int i = 0; i < static_cast<int>(items.size()); ++i) {
				if (items[i] && items[i]->getId() == REQUIRED_FISH_ID) {
					fish_index = i;
					break;
				}
			}

			if (fish_index != -1) {
				// Usuwamy rybę i dodajemy nagrodę do ekwipunku kota.
				backpack.removeItem(fish_index);

				if (const auto sword = player->getEngine()->getItemDatabase().createItem(CAT_SWORD_ID))
					addItem(sword);

				_quest_completed = true;

				// Informujemy system questów o dostarczeniu przedmiotu.
				player->getEngine()->getQuestManager().notifyItemDelivered(REQUIRED_FISH_ID, getName());
				player->getEngine()->getDialogueManager().createCatQuestCompletedDialogue(player->getEngine(), this);
				player->getEngine()->getUIHandler().showNotification(
					"Zadanie ukonczone! Otrzymano Miecz Kota.",
					4.0f);

				return;
			}
		}

		if (_is_open)
			return;

		_is_open = true;
	}

	void Cat::update(const float delta_time) {
		Entity::update(delta_time);
	}

	void Cat::render(const Camera3D& camera) {
		Entity::render(camera);
	}

	float Cat::getInteractionRange() {
		return 2.5f * 2.5f;
	}

	Item::Backpack* Cat::getInventory() {
		return _inventory.get();
	}

	void Cat::addItem(const std::shared_ptr<Item::Item>& item) const {
		_inventory->addItem(item);
	}

} // namespace Nawia::Entity
