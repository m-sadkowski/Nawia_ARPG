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
		Core::Engine* getEngine() const { return _engine; }

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
		void equipItemFromBackpack(int backpack_index);
		void unequipItem(Item::EquipmentSlot slot);

		[[nodiscard]] const Item::Backpack& getBackpack() const { return *_backpack; }
		Item::Backpack& getBackpack() { return *_backpack; }
		[[nodiscard]] const Item::Equipment& getEquipment() const { return *_equipment; }

		[[nodiscard]] int getGold() const { return _gold; }
		void addGold(const int amount) { _gold += amount; }
		bool spendGold(const int amount) {
			if (_gold >= amount) 
			{
				_gold -= amount;
				return true;
			}
			return false;
		}

		void recalculateStats();
		[[nodiscard]] const Stats& getStats() const { return _current_stats; }
		
		[[nodiscard]] int getLevel() const { return _level; }
		void setLevel(int level) { _level = level; }
		[[nodiscard]] int getExp() const { return _exp;  }
		void addExp(const int amount) { _exp += amount; }
		[[nodiscard]] int getExpToNextLvl() const { return _expToNextLvl; }
		
		void levelUp();
		void isLevelUp();

		//Constant values for animation
		
		static constexpr float DEFAULT_ANIMATION_SPEED = 1.0f;
		static constexpr float WALK_ANIM_BASE_SPEED = 1.25f;
		static constexpr float ATTACK_ANIM_BASE_SPEED = 2.0f;


	private:
		friend class PlayerBuilder;
		Player();

		Core::Engine* _engine;
		
		static constexpr int INIT_BACKPACK_SIZE = 20;
		float _target_x, _target_y;
		bool _is_moving = false;
		bool _is_knocked_down = false;
		enum class KnockdownPhase { None, Knocked, StandingUp };
		KnockdownPhase _knockdown_phase = KnockdownPhase::None;
		std::vector<Vector2> _path;

		std::unique_ptr<Item::Backpack> _backpack;
		std::unique_ptr<Item::Equipment> _equipment;

		Stats _base_stats;
		Stats _current_stats;

		int _gold = 0;
		int _level = 1;
		int _exp = 0;
		int _expToNextLvl=100;
	};

	class PlayerBuilder : public EntityBuilder<PlayerBuilder> {
	public:
		PlayerBuilder(Core::Engine* engine) {
			_player_ptr = std::unique_ptr<Player>(new Player());
			_player_ptr->_engine = engine;

			this->_entity = _player_ptr.get();
		}

		PlayerBuilder& setPosition(const Vector2 pos) {
			EntityBuilder<PlayerBuilder>::setPosition(pos);
			_player_ptr->_target_x = pos.x;
			_player_ptr->_target_y = pos.y;
			return *this;
		}

		PlayerBuilder& setX(const float x) {
			EntityBuilder<PlayerBuilder>::setX(x);
			_player_ptr->_target_x = x;
			return *this;
		}

		PlayerBuilder& setY(const float y) {
			EntityBuilder<PlayerBuilder>::setY(y);
			_player_ptr->_target_y = y;
			return *this;
		}

		std::unique_ptr<Player> build() {
			return std::move(_player_ptr);
		}
	private:
		std::unique_ptr<Player> _player_ptr;
	};

	

} // namespace Nawia::Entity