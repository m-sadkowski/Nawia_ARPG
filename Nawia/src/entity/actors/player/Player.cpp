#include "Player.h"
#include "Collider.h"

#include <Map.h>
#include <Constants.h>
#include <MathUtils.h>
#include <Engine.h>
#include <Logger.h>

#include <cmath>

namespace Nawia::Entity {

	Player::Player() {
		_name = "Player";
		_hp = 100;
		_max_hp = 100;
		_scale = 0.015f;
		_type = EntityType::Player;
		_faction = Faction::Player;
		loadModel("../assets/models/player_idle.glb");
		addAnimation("walk", "../assets/models/player_walk.glb");
		addAnimation("attack", "../assets/models/player_auto_attack.glb");
		addAnimation("knocked", "../assets/models/player_knocked.glb");
		addAnimation("stand_up", "../assets/models/player_stand_up.glb");
		playAnimation("default"); // play idle
		setAnimationSpeed(1.0f);

		// init backpack and eq
		_backpack = std::make_unique<Item::Backpack>(INIT_BACKPACK_SIZE);
		_equipment = std::make_unique<Item::Equipment>();

		_base_stats.max_hp = 100;
		_base_stats.damage = 10;
		_base_stats.attack_speed = 1.0f;
		_base_stats.movement_speed = 4.0f;
		_base_stats.tenacity = 0;

		recalculateStats();
	}

	void Player::moveTo(const float x, const float y)
	{
		_target_x = x;
		_target_y = y;

		const float dx = _target_x - getX();
		const float dy = _target_y - getY();
		
		if (dx * dx + dy * dy > 0.001f) 
		{
			_is_moving = true;
		} 
		else 
		{
			_is_moving = false;
			if (!isAnimationLocked())
			{
				setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
				playAnimation("default");
			}
			return;
		}

		if (_is_moving && !isAnimationLocked()) 
		{
			setAnimationSpeed(_current_stats.movement_speed * WALK_ANIM_BASE_SPEED);
			playAnimation("walk");
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
		updateAbilities(delta_time);

		// Handle dying state - wait for animation then truly die
		if (_is_dying)
		{
			if (!isAnimationLocked())
			{
				_hp = 0; // now truly dead - Engine will switch to GameOver
			}
			return;
		}
		
		// Handle knockdown animation sequence
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
			return; // Don't process movement while knocked
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

		// Calculate direction to target (straight-line movement)
		const float dx = _target_x - getX();
		const float dy = _target_y - getY();
		const float distance = std::sqrt(dx * dx + dy * dy);

		// Rotate towards target
		if (distance > 0.001f)
			rotateTowards(_target_x, _target_y);

		// Apply velocity
		const float speed = _current_stats.movement_speed;
		const float move_dist = speed * delta_time;

		if (move_dist >= distance) 
		{
			// Snap to target
			_pos.x = _target_x;
			_pos.y = _target_y;
			_is_moving = false;
			if (!isAnimationLocked()) playAnimation("default");
		} 
		else 
		{
			// Move normally
			_pos.x += (dx / distance) * move_dist;
			_pos.y += (dy / distance) * move_dist;
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

	void Player::takeDamage(const int dmg)
	{
		if (_is_dying) return;

		if (_hp - dmg <= 0)
		{
			_hp = 1; // keep alive for death animation
			_is_dying = true;
			stop();
			setAnimationSpeed(2.0f);
			playAnimation("knocked", false, true, 0, true);
		}
		else
		{
			Entity::takeDamage(dmg);
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