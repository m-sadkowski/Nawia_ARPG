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

		_target_x = x - offset_x;
		_target_y = y - offset_y;

		//Core::Logger::debugLog("Player::moveTo(org) -> x=" + std::to_string(x) + " y=" + std::to_string(y));

		if (!map->isWalkable(x, y)) {
			return;
		}

		_is_moving = true;

		if (!isAnimationLocked())
			setAnimationSpeed(_current_stats.movement_speed*WALK_ANIM_BASE_SPEED);
			playAnimation("walk");

		const float dx = _target_x - getX();
		const float dy = _target_y - getY();
		const float distance_sq = dx * dx + dy * dy;

		if (distance_sq > 0.001f)
		{
			rotateTowards(_target_x, _target_y);
		}
	}

	void Player::stop()
	{
		_is_moving = false;
		if (!isAnimationLocked())
		{
		if (!isAnimationLocked())
		{
			setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
			playAnimation("default");
			
		}
		}
	}

	void Player::update(const float delta_time)
	{
		Entity::update(delta_time);
		updateAbilities(delta_time);
		updateMovement(delta_time);
	}

	void Player::updateMovement(const float delta_time)
	{
		if (!_is_moving)
			return;

		const float dx = _target_x - _pos.x;
		const float dy = _target_y - _pos.y;
		const float distance = std::sqrt(dx * dx + dy * dy);

		if (!isAnimationLocked()) {
			setAnimationSpeed(_current_stats.movement_speed * WALK_ANIM_BASE_SPEED);
			playAnimation("walk");
			// Scale animation speed based on movement speed
			// This makes the walk animation faster as the player moves faster
			
		}

		if (distance < 0.1f)
		{
			_pos.x = _target_x;
			_pos.y = _target_y;
			_is_moving = false;

			if (!isAnimationLocked())
				playAnimation("default");
		}
		else
		{
			// Calculate next position
			float next_x = _pos.x + (dx / distance) * _current_stats.movement_speed * delta_time;
			float next_y = _pos.y + (dy / distance) * _current_stats.movement_speed * delta_time;

			// Check if next position is walkable (check center position)
			Core::Map* map = _engine->getCurrentMap();
			const Vector2 center = getCenter();
			float center_offset_x = center.x - getX();
			float center_offset_y = center.y - getY();
			float next_center_x = next_x + center_offset_x;
			float next_center_y = next_y + center_offset_y;

			// Temporary: simple check, can be improved with pathfinding
			if (map && !map->isWalkable(next_center_x, next_center_y)) {
				// Stop movement if next tile is not walkable
				_is_moving = false;
				if (!isAnimationLocked())
					playAnimation("default");
				return;
			}

			_pos.x = next_x;
			_pos.y = next_y;
		}
	}

	void Player::equipItemFromBackpack(int backpackIndex) {
		auto item = _backpack->getItem(backpackIndex);
		if (!item) return;

		_backpack->removeItem(backpackIndex);

		// equip item
		auto old_item = _equipment->equip(item);

		// if cannot equip go back to backpack
		if (old_item) {
			_backpack->addItem(old_item);
			// todo what if backpack full (player somehow picked up item while equip)
		}
		recalculateStats();
	}

	void Player::unequipItem(Item::EquipmentSlot slot) {
		auto item = _equipment->getItemAt(slot);
		if (!item) return;

		if (_backpack->getRemainingCapacity() > 0) {
			_backpack->addItem(item);
			_equipment->unequip(slot);
			recalculateStats();
		}
	}

	void Player::recalculateStats() {
		_current_stats = _base_stats;
		
		// Check all slots  
		for (int i = 1; i <= 8; ++i) {
			auto item = _equipment->getItemAt(static_cast<Item::EquipmentSlot>(i));
			if (item) {
				_current_stats += item->getStats();
			}
		}

		_max_hp = _current_stats.max_hp;
		if (_hp > _max_hp) _hp = _max_hp;
	}

} // namespace Nawia::Entity