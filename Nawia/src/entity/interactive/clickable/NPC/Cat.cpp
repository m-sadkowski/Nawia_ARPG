#include "Cat.h"

#include <Backpack.h>
#include <Collider.h>
#include <Engine.h>
#include <Loottable.h>
#include <Player.h>
#include <SoundIds.h>

namespace Nawia::Entity {

	namespace {
		constexpr int REQUIRED_FISH_ID = 6;
	}

	Cat::Cat(const std::string& name, const float x, const float y, const std::shared_ptr<Texture2D>& texture)
		: InteractiveClickable(name, x, y, texture, 1) // NPC ma techniczne 1 HP.
	{
		_type = EntityType::NPCStatic;
		setFaction(Faction::None);
		setScale(0.03f);
		loadModel("assets/models/cat_bounce.glb", false);
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
				// Usuwamy rybe, a nagrode nada system questow po dostarczeniu przedmiotu.
				backpack.removeItem(fish_index);

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
		playSoundEffect(Audio::SoundId::CatMeow, 0.8f);
	}

	void Cat::update(const float delta_time) {
		Entity::update(delta_time);
	}

	bool Cat::isMouseOver(const float screen_x, const float screen_y, const Camera3D& camera) const {
		if (Entity::isMouseOver(screen_x, screen_y, camera))
			return true;

		const Ray mouse_ray = GetScreenToWorldRay(Vector2{ screen_x, screen_y }, camera);
		const Vector3 pos = getWorldPos3D();
		constexpr float click_half_width = 0.7f;
		constexpr float click_height = 1.45f;

		const BoundingBox click_box = {
			Vector3{ pos.x - click_half_width, pos.y, pos.z - click_half_width },
			Vector3{ pos.x + click_half_width, pos.y + click_height, pos.z + click_half_width }
		};

		return GetRayCollisionBox(mouse_ray, click_box).hit;
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

	nlohmann::json Cat::serializeState() const {
		nlohmann::json state = Entity::serializeState();
		state["open"] = _is_open;
		state["quest_completed"] = _quest_completed;
		if (_inventory)
			state["inventory"] = _inventory->serialize();
		return state;
	}

	void Cat::applyState(const nlohmann::json& state, Item::ItemDatabase* item_database) {
		Entity::applyState(state, item_database);
		if (!state.is_object())
			return;

		_is_open = state.value("open", _is_open);
		_quest_completed = state.value("quest_completed", _quest_completed);

		if (item_database && _inventory && state.contains("inventory"))
			_inventory->applyJson(state["inventory"], *item_database);
	}

} // namespace Nawia::Entity
