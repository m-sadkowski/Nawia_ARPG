#include "Player.h"
#include <Collider.h>

#include <Constants.h>
#include <Engine.h>
#include <Logger.h>
#include <Map.h>
#include <MathUtils.h>

#include <cmath>

namespace Nawia::Entity {

	Player::Player() {
		_name = "Player";
		_max_hp = 200;
		_hp = _max_hp;
		_scale = 0.015f;
		_type = EntityType::Player;
		_faction = Faction::Player;
		loadModel("assets/models/player_idle.glb");
		addAnimation("walk", "assets/models/player_walk.glb");
		addAnimation("attack", "assets/models/player_auto_attack.glb");
		addAnimation("knocked", "assets/models/player_knocked.glb");
		addAnimation("stand_up", "assets/models/player_stand_up.glb");
		playAnimation("default"); // Startowa animacja bezczynności.
		setAnimationSpeed(1.0f);
		_death_anim_name = "knocked";

		// Inicjalizacja plecaka i ekwipunku.
		_backpack = std::make_unique<Item::Backpack>(INIT_BACKPACK_SIZE);
		_equipment = std::make_unique<Item::Equipment>();

		_base_stats.max_hp = _max_hp;
		_base_stats.damage = 10;
		_base_stats.attack_speed = 1.0f;
		_base_stats.movement_speed = 4.0f;
		_base_stats.tenacity = 0;

		recalculateStats();
	}

	void Player::moveTo(const float x, const float y)
	{
		Entity::moveTo(x, y);

		if (_is_moving) 
		{
			if (!isAnimationLocked())
			{
				setAnimationSpeed(_current_stats.movement_speed * WALK_ANIM_BASE_SPEED);
				playAnimation("walk");
			}
		} 
		else 
		{
			if (!isAnimationLocked())
			{
				setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
				playAnimation("default");
			}
		}
	}

	void Player::stop()
	{
		_is_moving = false;
		_path.clear();
		if (!isAnimationLocked())
		{
			setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
			playAnimation("default");
		}
	}

	void Player::update(const float delta_time)
	{
		Entity::update(delta_time);
		
		if (isDying()) return;

		updateAbilities(delta_time);
		
		isLevelUp();
		// Obsługa sekwencji powalenia.
		if (_is_knocked_down)
		{
			if (!isAnimationLocked())
			{
				if (_knockdown_phase == KnockdownPhase::Knocked)
				{
					_knockdown_phase = KnockdownPhase::StandingUp;
					playAnimation("stand_up", false, true, 0, true);
				}
				else
				{
					_is_knocked_down = false;
					_knockdown_phase = KnockdownPhase::None;
					setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
					playAnimation("default");
				}
			}
			return; // Podczas powalenia nie przetwarzamy ruchu.
		}
		
		updateMovement(delta_time);
	}

	void Player::updateMovement(const float delta_time)
	{
		if (!_is_moving || _is_knocked_down)
			return;

		if (!isAnimationLocked()) 
		{
			setAnimationSpeed(_current_stats.movement_speed * WALK_ANIM_BASE_SPEED);
			playAnimation("walk");
		}

		Entity::updateMovement(delta_time);

		if (!_is_moving && !isAnimationLocked()) 
		{
			playAnimation("default");
		}
	}

	void Player::equipItemFromBackpack(const int backpack_index) 
	{
		const auto item = _backpack->getItem(backpack_index);
		if (!item) return;

		_backpack->removeItem(backpack_index);

		if (const auto old_item = _equipment->equip(item)) 
			_backpack->addItem(old_item);

		recalculateStats();
	}

	void Player::unequipItem(const Item::EquipmentSlot slot) 
	{
		const auto item = _equipment->getItemAt(slot);
		if (!item) return;

		if (_backpack->getRemainingCapacity() > 0) {
			_backpack->addItem(item);
			_equipment->unequip(slot);
			recalculateStats();
		}
	}

	void Player::recalculateStats() 
	{
		_current_stats = _base_stats;
		
		for (int i = 1; i <= 8; ++i) 
		{
			if (const auto item = _equipment->getItemAt(static_cast<Item::EquipmentSlot>(i)))
				_current_stats += item->getStats();
		}

		_max_hp = _current_stats.max_hp;
		_hp = std::min(_hp, _max_hp);
		_movement_speed = _current_stats.movement_speed;
	}

	void Player::knockDown(const int damage)
	{
		if (_is_knocked_down)
		{
			takeDamage(damage);
			return;
		}

		stop();
		takeDamage(damage);

		_is_knocked_down = true;
		_knockdown_phase = KnockdownPhase::Knocked;
		setAnimationSpeed(4.0f);
		playAnimation("knocked", false, true, 0, true);
	}

	void Player::levelUp() {
		_level++;
    _exp = _exp - _exp_to_next_lvl;
    _exp_to_next_lvl = _exp_to_next_lvl + 1000;
		_base_stats.max_hp = _base_stats.max_hp + 15;
		_base_stats.damage = _base_stats.damage + 2;
		_base_stats.attack_speed = _base_stats.attack_speed + 0.1f;
		_base_stats.movement_speed = 4.0f;
		_base_stats.tenacity = _base_stats.tenacity + 0.1f;
		recalculateStats();
	}

	void Player::isLevelUp() {
    if (_exp >= _exp_to_next_lvl) levelUp();
	}

	void Player::takeDamage(const int dmg)
	{
		const bool was_dying = isDying();
		Entity::takeDamage(dmg);

		if (!was_dying && isDying())
		{
			stop();
			setAnimationSpeed(2.0f);
		}
	}

	void Player::respawn()
	{
		_hp = _max_hp;
		_is_dying = false;
		_is_knocked_down = false;
		_knockdown_phase = KnockdownPhase::None;
		_is_moving = false;
		setX(_respawn_point.x);
		setY(_respawn_point.y);
		_target_x = _respawn_point.x;
		_target_y = _respawn_point.y;
		_path.clear();
		setFaction(Faction::Player);
		setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
		playAnimation("default");
	}

} // namespace Nawia::Entity
