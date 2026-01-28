#pragma once
#include "Entity.h"
#include "Ability.h"
#include "Stats.h"
#include <Backpack.h>
#include <Equipment.h>

#include <memory>
#include <vector>

namespace Nawia::Core {
	class Engine;
}

namespace Nawia::Entity {

	class Player : public Entity {
	public:
		Player(Core::Engine* engine, float x, float y, const std::shared_ptr<Texture2D>& texture);

		void update(float delta_time) override;
		[[nodiscard]] bool isMoving() const { return _is_moving; }
		[[nodiscard]] bool isKnockedDown() const { return _is_knocked_down; }

		void moveTo(float x, float y);
		void stop();
		void updateMovement(float delta_time);
		
		/**
		 * @brief Knock the player down (e.g. from Devil dash attack).
		 * Plays knocked animation followed by stand_up animation.
		 * Movement is blocked during this sequence.
		 * @param damage Amount of damage to deal
		 */
		void knockDown(int damage);


		void equipItemFromBackpack(int backpackIndex);
		void unequipItem(Item::EquipmentSlot slot);

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

		void recalculateStats();
		const Stats& getStats() const { return _current_stats; }


		//Constant values for animation
		
		static constexpr float DEFAULT_ANIMATION_SPEED = 1.0f;
		static constexpr float WALK_ANIM_BASE_SPEED = 1.25f;
		static constexpr float ATTACK_ANIM_BASE_SPEED = 2.0f;


	private:
		Core::Engine* _engine;
		
		static constexpr int INIT_BACKPACK_SIZE = 20;
		float _target_x, _target_y;
		bool _is_moving;
		bool _is_knocked_down = false;
		enum class KnockdownPhase { None, Knocked, StandingUp };
		KnockdownPhase _knockdown_phase = KnockdownPhase::None;
		std::vector<Vector2> _path;

		std::unique_ptr<Item::Backpack> _backpack;
		std::unique_ptr<Item::Equipment> _equipment;

		Stats _base_stats;
		Stats _current_stats;

		int _gold = 0;
	};

} // namespace Nawia::Entity