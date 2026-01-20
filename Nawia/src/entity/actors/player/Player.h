#pragma once
#include "Entity.h"
#include "Ability.h"
#include <Backpack.h>
#include <Equipment.h>

#include <memory>
#include <vector>

namespace Nawia::Entity {

	class Player : public Entity {
	public:
		Player(float x, float y, const std::shared_ptr<Texture2D>& texture);

		void update(float delta_time) override;
		[[nodiscard]] bool isMoving() const { return _is_moving; }

		void moveTo(float x, float y);
		void stop();
		void updateMovement(float delta_time);
		
		void equipItemFromBackpack(int backpackIndex) const;
		void unequipItem(Item::EquipmentSlot slot) const;

		const Item::Backpack& getBackpack() const { return *_backpack; }
		Item::Backpack& getBackpack() { return *_backpack; }
		const Item::Equipment& getEquipment() const { return *_equipment; }

		int getGold() const { return _gold; }
		void addGold(int amount) { _gold += amount; }
		bool spendGold(int amount) {
			if (_gold >= amount) {
				_gold -= amount;
				return true;
			}
			return false;
		}
	private:
		static constexpr int INIT_BACKPACK_SIZE = 20;

		float _target_x, _target_y;
		float _speed;
		bool _is_moving;

		std::unique_ptr<Item::Backpack> _backpack;
		std::unique_ptr<Item::Equipment> _equipment;

		int _gold = 0;
	};

} // namespace Nawia::Entity