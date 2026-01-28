#include "Player.h"
#include "Collider.h"

#include <Constants.h>
#include <MathUtils.h>
#include <Engine.h>
#include <Logger.h>

#include <cmath>

namespace Nawia::Entity {

	Player::Player(Core::Engine* engine, const float x, const float y, const std::shared_ptr<Texture2D>& texture)
	    : Entity("Player", x, y, texture, 100), _target_x(x), _target_y(y),  _is_moving(false) 
	{
		_engine = engine;
		this->setScale(0.015f);
		_type = EntityType::Player;
		setFaction(Faction::Player);
		loadModel("../assets/models/player_idle.glb");
		addAnimation("walk", "../assets/models/player_walk.glb");
		addAnimation("attack", "../assets/models/player_auto_attack.glb");
		addAnimation("knocked", "../assets/models/player_knocked.glb");
		addAnimation("stand_up", "../assets/models/player_stand_up.glb");
		playAnimation("default"); // play idle
		setAnimationSpeed(1.0f);
		// add collider
		setCollider(std::make_unique<RectangleCollider>(this, 0.3f, 0.8f, -2.1f, -1.f));

		// init backpack and eq
		_backpack = std::make_unique<Item::Backpack>(INIT_BACKPACK_SIZE);
		_equipment = std::make_unique<Item::Equipment>();

		_base_stats.max_hp = 100;
		_base_stats.damage = 10;
		_base_stats.attack_speed = 1.0f;
		_base_stats.movement_speed = 4.0f;  // Base movement speed
		_base_stats.tenacity = 0;

		recalculateStats();
	}

	void Player::moveTo(const float x, const float y)
	{
		// Adjusted target so that the CENTER of the collider lands on the click point, not the feet.
		const Vector2 center = getCenter();
		const float offset_x = center.x - getX();
		const float offset_y = center.y - getY();

		Core::Map* map = _engine->getCurrentMap();

		const Vector2 start_world = { center.x, center.y };
		// We want the center to end up at (x, y)
		const Vector2 end_world = { x, y };

		// Check if target is walkable
		if (map && !map->isWalkable(x, y))
			return;

		_target_x = x - offset_x;
		_target_y = y - offset_y;
		
		if (map) 
		{
			_path = map->findPath(start_world, end_world);
			if (!_path.empty())
				// Ensure the final point is exactly the requested pixel coordinates
				_path.back() = end_world;
			
		} 
		else 
		{
			_path.clear();
		}

		if (_path.empty()) 
		{
			// If pathfinding failed just stop
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
		} 
		else 
		{
			_is_moving = true;
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
		
		Vector2 current_target_world;
		
		if (!_path.empty()) 
		{
			// Move towards next path node
			current_target_world = _path.front();
		} 
		else 
		{
			// Move towards final request target
			const Vector2 center = getCenter();
			const float offset_x = center.x - getX();
			const float offset_y = center.y - getY();
			current_target_world.x = _target_x + offset_x;
			current_target_world.y = _target_y + offset_y;
		}

		// Calculate direction
		const Vector2 center = getCenter();
		const float dx = current_target_world.x - center.x;
		const float dy = current_target_world.y - center.y;
		const float distance_sq = dx * dx + dy * dy;
		const float distance = std::sqrt(distance_sq);

		// Rotate towards target
		if (distance_sq > 0.001f)
			rotateTowardsCenter(current_target_world.x, current_target_world.y);

		// Move
		// If we are close enough to current target node
		if (distance < 0.1f * (_current_stats.movement_speed / 4.0f)) // threshold proportional to speed
		{ 
			if (!_path.empty()) {
				// Reached this node, pop it
				_path.erase(_path.begin());
			} 
			else 
			{
				// Reached final target
				_is_moving = false;
				_pos.x = _target_x;
				_pos.y = _target_y;
				
				if (!isAnimationLocked())
					playAnimation("default");
				return;
			}
		}

		// Apply velocity
		const float speed = _current_stats.movement_speed;
		const float move_dist = speed * delta_time;

		if (move_dist >= distance && _path.empty()) 
		{
			// We will overshoot final target, just snap
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
			// todo what if backpack full (player somehow picked up item while equip)

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
		
		// Check all slots  
		for (int i = 1; i <= 8; ++i) 
		{
			if (const auto item = _equipment->getItemAt(static_cast<Item::EquipmentSlot>(i)))
				_current_stats += item->getStats();
		}

		_max_hp = _current_stats.max_hp;
		if (std::min(_hp, _max_hp))
			_hp = _max_hp;
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

} // namespace Nawia::Entity