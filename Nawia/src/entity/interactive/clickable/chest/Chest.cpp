#include "Chest.h"

#include <Backpack.h>
#include <Collider.h>
#include <Engine.h>
#include <Loottable.h>
#include <Player.h>
#include <SoundIds.h>

#include <algorithm>


namespace Nawia::Entity {

	namespace {
		constexpr const char* CHEST_MODEL_LOCKED = "assets/models/chest/chest_close.glb";
		constexpr const char* CHEST_MODEL_OPEN_WITH_LOOT = "assets/models/chest/chest_open_full.glb";
		constexpr const char* CHEST_MODEL_EMPTY = "assets/models/chest/chest_open.glb";
	}

	Chest::Chest(const std::string& name, const float x, const float y, const std::shared_ptr<Texture2D>& texture)
		: InteractiveClickable(name, x, y, texture, 1) // Skrzynia ma techniczne 1 HP.
	{
		_type = EntityType::Chest;
		setFaction(Faction::None);
		setScale(1.0f);
		setCollider(std::make_unique<RectangleCollider>(this, 0.9f, 0.4f, 0.0f, 0.0f));

		_inventory = std::make_unique<Item::Backpack>(INVENTORY_SIZE);
		refreshVisualModel();
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
		auto* player = dynamic_cast<Player*>(&instigator);

		if (!_locked && isEmpty()) {
			if (player)
				player->getEngine()->getUIHandler().showNotification("Ta skrzynia jest pusta", 3.0f);
			return;
		}

		if (_is_open) {
			if (player)
				player->getEngine()->getUIHandler().showNotification("Skrzynia jest juz otwarta.");
			return;
		}

		if (_locked) {
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
		refreshVisualModel();
		playSoundEffect(Audio::SoundId::ChestOpen, 0.85f);
	}

	void Chest::update(const float delta_time) {
		refreshVisualModel();
		Entity::update(delta_time);
	}

	void Chest::render(const Camera3D& camera) {
		Entity::render(camera);
	}

	float Chest::getInteractionRange() {
		return 2.5f * 2.5f;
	}

	Item::Backpack* Chest::getInventory() {
		if (_locked || isEmpty())
			return nullptr;

		return _inventory.get();
	}

	void Chest::addItem(const std::shared_ptr<Item::Item>& item) {
		_inventory->addItem(item);
		refreshVisualModel();
	}

	void Chest::setLocked(const bool locked, const int key_id) {
		_locked = locked;
		_key_id = key_id;
		refreshVisualModel();
	}

	void Chest::setOpen(const bool open) {
		_is_open = open;
		refreshVisualModel();
	}

	bool Chest::isEmpty() const {
		if (!_inventory)
			return true;

		const auto& items = _inventory->getItems();
		return std::ranges::none_of(items, [](const std::shared_ptr<Item::Item>& item) {
			return item != nullptr;
		});
	}

	const char* Chest::getVisualModelPath() const {
		if (_locked)
			return CHEST_MODEL_LOCKED;

		return isEmpty() ? CHEST_MODEL_EMPTY : CHEST_MODEL_OPEN_WITH_LOOT;
	}

	void Chest::refreshVisualModel() {
		const char* model_path = getVisualModelPath();
		if (_active_model_path == model_path)
			return;

		_active_model_path = model_path;
		loadModel(_active_model_path);
	}

	nlohmann::json Chest::serializeState() const {
		nlohmann::json state = Entity::serializeState();
		state["open"] = _is_open;
		state["locked"] = _locked;
		state["key_id"] = _key_id;
		if (_inventory)
			state["inventory"] = _inventory->serialize();
		return state;
	}

	void Chest::applyState(const nlohmann::json& state, Item::ItemDatabase* item_database) {
		Entity::applyState(state, item_database);
		if (!state.is_object())
			return;

		_is_open = state.value("open", _is_open);
		_locked = state.value("locked", _locked);
		_key_id = state.value("key_id", _key_id);

		if (item_database && _inventory && state.contains("inventory"))
			_inventory->applyJson(state["inventory"], *item_database);

		refreshVisualModel();
	}

} // namespace Nawia::Entity
