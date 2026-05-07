#include "Chest.h"

#include <Backpack.h>
#include <Collider.h>
#include <Engine.h>
#include <Loottable.h>
#include <Player.h>

namespace Nawia::Entity {

	Chest::Chest(const std::string& name, const float x, const float y, const std::shared_ptr<Texture2D>& texture)
		: InteractiveClickable(name, x, y, texture, 1) // Skrzynia ma techniczne 1 HP.
	{
		_type = EntityType::Chest;
		setFaction(Faction::None);
		loadModel("assets/models/fireball.glb");
		setScale(0.35f);
		setCollider(std::make_unique<RectangleCollider>(this, 0.9f, 0.4f, 0.0f, 0.0f));

		_inventory = std::make_unique<Item::Backpack>(INVENTORY_SIZE);
	}

	Chest::~Chest() = default;

	void Chest::initializeInventory(Item::Loottable& loot_table, const Item::LOOTTABLE_TYPE loot_table_type) {
		const auto& drops = loot_table.getLootTable(loot_table_type);

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

	void Chest::onInteract(Entity& instigator) {
		if (_is_open) {
			if (const auto* player = dynamic_cast<Player*>(&instigator))
				player->getEngine()->getUIHandler().showNotification("Skrzynia jest juz otwarta.");

			return;
		}

		if (_locked) {
			auto* player = dynamic_cast<Player*>(&instigator);
			if (!player)
				return;

			bool has_key = false;
			int key_index = -1;
			auto& backpack = player->getBackpack();
			const auto& items = backpack.getItems();

			for (int i = 0; i < static_cast<int>(items.size()); ++i) {
				if (items[i] && items[i]->getId() == _key_id) {
					key_index = i;
					has_key = true;
					break;
				}
			}

			auto& ui = player->getEngine()->getUIHandler();
			if (!has_key) {
				ui.showNotification("Skrzynia zamknieta! Potrzebny Klucz Kota.", 3.0f);
				return;
			}

			backpack.removeItem(key_index);
			ui.showNotification("Skrzynia otwarta! Zuzyto Klucz Kota.", 2.5f);
			_locked = false;
		}

		// Tu można później uruchomić animację otwierania skrzyni.
		_is_open = true;
	}

	void Chest::update(const float delta_time) {
		Entity::update(delta_time);
	}

	void Chest::render(const Camera3D& camera) {
		Entity::render(camera);
	}

	float Chest::getInteractionRange() {
		return 2.5f * 2.5f;
	}

	Item::Backpack* Chest::getInventory() {
		if (_locked)
			return nullptr;

		return _inventory.get();
	}

	void Chest::addItem(const std::shared_ptr<Item::Item>& item) {
		_inventory->addItem(item);
	}

	void Chest::setLocked(const bool locked, const int key_id) {
		_locked = locked;
		_key_id = key_id;
	}

} // namespace Nawia::Entity
