#include "Player.h"
#include <Collider.h>

#include <Constants.h>
#include <Engine.h>
#include <FireballAbility.h>
#include <ItemDatabase.h>
#include <Logger.h>
#include <Map.h>
#include <MathUtils.h>
#include <PlayerAbilityFactory.h>
#include <SoundIds.h>
#include <SwordSlashAbility.h>

#include <algorithm>
#include <cmath>

namespace Nawia::Entity {

	namespace {
		constexpr const char* PLAYER_HEAD_MODEL = "assets/models/actors/player/parts/player_head.glb";
		constexpr const char* PLAYER_HEAD_WITH_SWORD_MODEL = "assets/models/items/player_head_with_sword.glb";
		constexpr const char* FIREBALL_MODEL = "assets/models/fireball.glb";
		constexpr const char* FIREBALL_ICON = "assets/textures/icons/fireball_icon.png";
		constexpr int FIREBALL_ABILITY_SLOT = 2;

		nlohmann::json statsToJson(const Stats& stats) {
			return {
				{"max_hp", stats.max_hp},
				{"damage", stats.damage},
				{"power", stats.power},
				{"attack_speed", stats.attack_speed},
				{"movement_speed", stats.movement_speed},
				{"defense", stats.defense}
			};
		}

		Stats statsFromJson(const nlohmann::json& data, const Stats& fallback) {
			if (!data.is_object())
				return fallback;

			Stats stats = fallback;
			stats.max_hp = data.value("max_hp", stats.max_hp);
			stats.damage = data.value("damage", stats.damage);
			stats.power = data.value("power", stats.power);
			stats.attack_speed = data.value("attack_speed", stats.attack_speed);
			stats.movement_speed = data.value("movement_speed", stats.movement_speed);
			stats.defense = data.value("defense", stats.defense);
			return stats;
		}

		nlohmann::json vector2ToJson(const Vector2 value) {
			return {{"x", value.x}, {"y", value.y}};
		}

		Vector2 vector2FromJson(const nlohmann::json& data, const Vector2 fallback) {
			if (!data.is_object())
				return fallback;

			return {data.value("x", fallback.x), data.value("y", fallback.y)};
		}
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
		if (isMovementRooted())
		{
			stop();
			return;
		}

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

	void Player::applyRoot(const float duration)
	{
		Entity::applyRoot(duration);
		_path.clear();
		updateMovementSound(Audio::SoundPath::Footsteps, false);
		if (!isAnimationLocked())
		{
			setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
			playAnimation("Idle_Loop");
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

	void Player::clearControlLocks()
	{
		clearStatusEffects();
		_is_knocked_down = false;
		_knockdown_phase = KnockdownPhase::None;
		_is_consuming_food = false;
		_consume_food_timer = 0.0f;
		stop();
		setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
		if (!isDying())
			playAnimation("Idle_Loop", true, false, 0, true);
	}

	void Player::update(const float delta_time)
	{
		Entity::update(delta_time);
		
		if (isDying()) return;

		updateAbilities(delta_time);

		if (_is_consuming_food)
		{
			_consume_food_timer -= delta_time;
			if (_consume_food_timer <= 0.0f)
			{
				_is_consuming_food = false;
				_consume_food_timer = 0.0f;
				(void)consumeFood();
				setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
				if (!isAnimationLocked())
					playAnimation("Idle_Loop");
			}
			return;
		}
		
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
		if (!_is_moving || _is_knocked_down || isMovementRooted())
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
		updatePrimaryAttackAbility();
		recalculateStats();

	}

	bool Player::equipItem(const std::shared_ptr<Item::Item>& item)
	{
		if (!item || !_equipment)
			return false;

		if (const auto old_item = _equipment->equip(item))
			_backpack->addItem(old_item);

		updateWeaponVisualModel();
		updatePrimaryAttackAbility();
		recalculateStats();
		return true;
	}

	void Player::addFood(const int amount) {
		_food_count = std::max(0, _food_count + amount);
	}

	bool Player::consumeFood() {
		if (_food_count <= 0 || isDying() || _hp >= _max_hp)
			return false;

		_food_count--;
		setHP(std::min(_max_hp, _hp + 25));
		if (_engine)
			_engine->getUIHandler().showNotification("Zjedzono zapasy: +25 HP", 2.0f);
		return true;
	}

	bool Player::startConsumeFood() {
		if (_is_consuming_food || _food_count <= 0 || isDying() || _hp >= _max_hp || isAnimationLocked())
			return false;

		stop();
		_is_consuming_food = true;
		_consume_food_timer = 1.0f;
		playSoundEffect(Audio::SoundId::PlayerEatSupplies, 0.85f);
		setAnimationSpeed(1.0f);
		if (getAnimationFrameCount("Consume") > 0)
			playAnimation("Consume", false, true, 0, true);
		else
			playAnimation("Interact", false, true, 0, true);
		return true;
	}

	bool Player::unlockFireballAbility(const bool show_notification) {
		if (_fireball_unlocked) {
			ensureUnlockedFireballAbility();
			return false;
		}

		_fireball_unlocked = true;
		ensureUnlockedFireballAbility();
		if (show_notification && _engine)
			_engine->getUIHandler().showNotification("Nauczono zaklecia: Fireball", 4.0f);
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
			updatePrimaryAttackAbility();
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

	void Player::setBaseStats(const Stats& stats)
	{
		_base_stats = stats;
		recalculateStats();
	}

	void Player::clearItems()
	{
		if (_backpack)
			_backpack->clear();

		if (_equipment)
			_equipment->clear();

		_food_count = 0;
		updateWeaponVisualModel();
		updatePrimaryAttackAbility();
		recalculateStats();
	}

	void Player::updatePrimaryAttackAbility()
	{
		if (!_engine || !_equipment)
			return;

		const bool has_weapon = _equipment->getItemAt(Item::EquipmentSlot::Weapon) != nullptr;
		if (has_weapon) {
			const auto icon = _engine->getResourceManager().getTexture("assets/textures/icons/sword_slash_icon.png");
			setAbility(0, std::make_shared<SwordSlashAbility>(nullptr, icon));
			return;
		}

		if (const auto punch = PlayerAbilityFactory::createUnarmedAbilityByName(
			PlayerAbilityFactory::getPlayerSetupConfig(),
			_engine->getResourceManager(),
			"Punch")) {
			setAbility(0, punch);
		}
	}

	void Player::ensureUnlockedFireballAbility() {
		if (!_fireball_unlocked || !_engine)
			return;

		if (const auto existing = getAbility(FIREBALL_ABILITY_SLOT); existing && existing->getName() == "Fireball")
			return;

		const auto icon = _engine->getResourceManager().getTexture(FIREBALL_ICON);
		setAbility(FIREBALL_ABILITY_SLOT, std::make_shared<FireballAbility>(
			FIREBALL_MODEL,
			0.5f,
			nullptr,
			icon,
			&_engine->getResourceManager()));
	}

	nlohmann::json Player::serializeProfile() const
	{
		return {
			{"level", _level},
			{"exp", _exp},
			{"exp_to_next_level", _exp_to_next_lvl},
			{"gold", _gold},
			{"food_count", _food_count},
			{"fireball_unlocked", _fireball_unlocked},
			{"base_stats", statsToJson(_base_stats)},
			{"inventory", _backpack ? _backpack->serialize() : nlohmann::json::object()},
			{"equipment", _equipment ? _equipment->serialize() : nlohmann::json::array()}
		};
	}

	void Player::applyProfile(const nlohmann::json& data, Item::ItemDatabase& item_database)
	{
		clearItems();

		if (!data.is_object())
			return;

		_level = data.value("level", _level);
		_exp = data.value("exp", _exp);
		_exp_to_next_lvl = data.value("exp_to_next_level", _exp_to_next_lvl);
		_gold = data.value("gold", _gold);
		_food_count = data.value("food_count", _food_count);
		_fireball_unlocked = data.value("fireball_unlocked", _fireball_unlocked);
		_base_stats = statsFromJson(data.value("base_stats", nlohmann::json::object()), _base_stats);

		if (data.contains("inventory") && _backpack)
			_backpack->applyJson(data["inventory"], item_database);

		if (data.contains("equipment") && data["equipment"].is_array()) {
			for (const auto& entry : data["equipment"]) {
				const int item_id = entry.value("item_id", 0);
				if (item_id <= 0)
					continue;

				if (auto item = item_database.createItem(item_id))
					equipItem(item);
			}
		}

		recalculateStats();
		ensureUnlockedFireballAbility();
	}

	nlohmann::json Player::serializeLocationView() const
	{
		const int safe_hp = isDead() ? std::max(1, _max_hp / 2) : std::clamp(_hp, 1, _max_hp);
		return {
			{"position", vector2ToJson({_pos.x, _pos.y})},
			{"altitude", _altitude},
			{"hp", safe_hp},
			{"max_hp", _max_hp},
			{"respawn_point", vector2ToJson(_respawn_point)}
		};
	}

	void Player::applyLocationView(const nlohmann::json& data)
	{
		if (!data.is_object())
			return;

		const Vector2 position = vector2FromJson(data.value("position", nlohmann::json::object()), {_pos.x, _pos.y});
		setX(position.x);
		setY(position.y);
		_altitude = data.value("altitude", _altitude);
		_respawn_point = vector2FromJson(data.value("respawn_point", nlohmann::json::object()), _respawn_point);

		setHP(data.value("hp", _hp));
		stop();
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
		clearStatusEffects();
		_hp = std::max(1, _max_hp / 2);
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
