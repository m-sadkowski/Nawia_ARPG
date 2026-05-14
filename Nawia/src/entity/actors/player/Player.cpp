#include "Player.h"
#include <Collider.h>

#include <Constants.h>
#include <Engine.h>
#include <Logger.h>
#include <Map.h>
#include <MathUtils.h>
#include <SoundIds.h>

#include <algorithm>
#include <cmath>

namespace Nawia::Entity {

	namespace {
		constexpr const char* PLAYER_HEAD_MODEL = "assets/models/player/player_head.glb";
		constexpr const char* PLAYER_HEAD_WITH_SWORD_MODEL = "assets/models/items/player_head_with_sword.glb";
	}

	Player::Player() {
		_name = "Player";
		_max_hp = 200;
		_hp = _max_hp;
		_scale = 1.5f;
		_type = EntityType::Player;
		_faction = Faction::Player;
		_active_visual_model_path = PLAYER_HEAD_MODEL;
		loadModel(_active_visual_model_path);
		loadAnimationBundle("assets/models/animations/anims.glb");
		loadAnimationBundle("assets/models/animations/anims2.glb");
		playAnimation("Idle_Loop");
		setAnimationSpeed(1.0f);
		_death_anim_name = "Death01";

		_backpack = std::make_unique<Item::Backpack>(INIT_BACKPACK_SIZE);

		_base_stats.max_hp = _max_hp;
		_base_stats.damage = 10;
		_base_stats.attack_speed = 1.0f;
		_base_stats.movement_speed = 4.0f;
		_base_stats.defense = 0;

		_current_stats = _base_stats;
		_movement_speed = _current_stats.movement_speed;
	}

	void Player::attachEngine(Core::Engine* engine) {
		_engine = engine;
		if (!_engine) return;

		_equipment = std::make_unique<Item::Equipment>(_engine->getResourceManager());
		updateWeaponVisualModel();
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
				playAnimation("Walk_Loop");
			}
		} 
		else 
		{
			if (!isAnimationLocked())
			{
				setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
				playAnimation("Idle_Loop");
			}
		}
	}

	void Player::stop()
	{
		_is_moving = false;
		_path.clear();
		updateMovementSound(Audio::SoundPath::Footsteps, false);
		if (!isAnimationLocked())
		{
			setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
			playAnimation("Idle_Loop");
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
					playAnimation("LayToIdle", false, true, 0, true);
				}
				else
				{
					_is_knocked_down = false;
					_knockdown_phase = KnockdownPhase::None;
					setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
					playAnimation("Idle_Loop");
				}
			}
			return; // Podczas powalenia nie przetwarzamy ruchu.
		}
		
		updateMovement(delta_time);
	}

	void Player::updateMovement(const float delta_time)
	{
		if (!_is_moving || _is_knocked_down)
		{
			updateMovementSound(Audio::SoundPath::Footsteps, false);
			return;
		}

		if (!isAnimationLocked()) 
		{
			setAnimationSpeed(_current_stats.movement_speed * WALK_ANIM_BASE_SPEED);
			playAnimation("Walk_Loop");
		}

		Entity::updateMovement(delta_time);
		updateMovementSound(Audio::SoundPath::Footsteps, _is_moving && !isAnimationLocked(), 0.48f);

		if (!_is_moving && !isAnimationLocked()) 
		{
			playAnimation("Idle_Loop");
		}
	}

	void Player::equipItemFromBackpack(const int backpack_index) 
	{
		const auto item = _backpack->getItem(backpack_index);
		if (!item) return;

		_backpack->removeItem(backpack_index);

		if (const auto old_item = _equipment->equip(item)) 
			_backpack->addItem(old_item);

		updateWeaponVisualModel();
		playSoundEffect(Audio::SoundId::ItemEquip, 0.85f);
		recalculateStats();

	}

	bool Player::equipItem(const std::shared_ptr<Item::Item>& item)
	{
		if (!item || !_equipment)
			return false;

		if (const auto old_item = _equipment->equip(item))
			_backpack->addItem(old_item);

		updateWeaponVisualModel();
		recalculateStats();
		return true;
	}

	void Player::unequipItem(const Item::EquipmentSlot slot) 
	{
		const auto item = _equipment->getItemAt(slot);
		if (!item) return;

		if (_backpack->getRemainingCapacity() > 0) {
			_backpack->addItem(item);
			_equipment->unequip(slot);
			updateWeaponVisualModel();
			recalculateStats();
		}
	}

	void Player::updateWeaponVisualModel()
	{
		if (!_equipment)
			return;

		const bool has_weapon = _equipment->getItemAt(Item::EquipmentSlot::Weapon) != nullptr;
		const std::string target_model = has_weapon ? PLAYER_HEAD_WITH_SWORD_MODEL : PLAYER_HEAD_MODEL;
		if (_active_visual_model_path == target_model)
			return;

		replaceModel(target_model);
		if (_model_loaded)
			_active_visual_model_path = target_model;
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
		playAnimation("Hit_Knockback", false, true, 0, true);
	}

	void Player::levelUp() {
		_level++;
    _exp = _exp - _exp_to_next_lvl;
    _exp_to_next_lvl = _exp_to_next_lvl + 1000;
		_base_stats.max_hp = _base_stats.max_hp + 15;
		_base_stats.damage = _base_stats.damage + 2;
		_base_stats.attack_speed = _base_stats.attack_speed + 0.1f;
		_base_stats.movement_speed = 4.0f;
		recalculateStats();
	}

	void Player::isLevelUp() {
    if (_exp >= _exp_to_next_lvl) levelUp();
	}

	void Player::takeDamage(const int dmg)
	{
		const bool was_dying = isDying();
		const float damage_reduction = std::clamp(static_cast<float>(_current_stats.defense) * 0.005f, 0.0f, 0.9f);
		const int reduced_damage = dmg > 0
			? std::max(1, static_cast<int>(std::round(static_cast<float>(dmg) * (1.0f - damage_reduction))))
			: dmg;

		Entity::takeDamage(reduced_damage);
		if (!isDying()) playSoundEffect(Audio::SoundId::PlayerHurt, 0.85f);


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
		playAnimation("Idle_Loop");
	}

	void Player::onDeathStarted()
	{
		updateMovementSound(Audio::SoundPath::Footsteps, false);
		playSoundEffect(Audio::SoundId::HumanDeath, 0.9f);
	}

} // namespace Nawia::Entity
