#include "Player.h"
#include "PlayerInternal.h"

#include <Engine.h>
#include <FireballAbility.h>
#include <ItemDatabase.h>
#include <PlayerAbilityFactory.h>
#include <SoundIds.h>
#include <SwordSlashAbility.h>
#include <UIHandler.h>

#include <algorithm>
#include <memory>

namespace Nawia::Entity {

	void Player::equipItemFromBackpack(const int backpack_index) {
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

	bool Player::equipItem(const std::shared_ptr<Item::Item>& item) {
		if (!item || !_equipment)
			return false;

		if (const auto old_item = _equipment->equip(item))
			_backpack->addItem(old_item);

		updateWeaponVisualModel();
		updatePrimaryAttackAbility();
		recalculateStats();
		return true;
	}

	void Player::unequipItem(const Item::EquipmentSlot slot) {
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

	void Player::addFood(const int amount) {
		_food_count = std::max(0, _food_count + amount);
	}

	bool Player::consumeFood() {
		if (_food_count <= 0 || isDying() || getHP() >= getMaxHP())
			return false;

		_food_count--;
		setHP(std::min(getMaxHP(), getHP() + 25));
		if (_engine)
			_engine->getUIHandler().showNotification("Zjedzono zapasy: +25 HP", 2.0f);
		return true;
	}

	bool Player::startConsumeFood() {
		if (_is_consuming_food || isControlLocked() || _food_count <= 0 || isDying() || getHP() >= getMaxHP() || isAnimationLocked())
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

	void Player::updateWeaponVisualModel() {
		if (!_equipment)
			return;

		const bool has_weapon = _equipment->getItemAt(Item::EquipmentSlot::Weapon) != nullptr;
		const std::string target_model = has_weapon
			? PlayerDetail::PLAYER_HEAD_WITH_SWORD_MODEL
			: PlayerDetail::PLAYER_HEAD_MODEL;
		if (_active_visual_model_path == target_model)
			return;

		replaceModel(target_model);
		if (hasModelLoaded())
			_active_visual_model_path = target_model;
	}

	void Player::recalculateStats() {
		_current_stats = _base_stats;

		for (int i = 1; i <= 8; ++i) {
			if (const auto item = _equipment->getItemAt(static_cast<Item::EquipmentSlot>(i)))
				_current_stats += item->getStats();
		}

		setMaxHpPreservingCurrentHp(_current_stats.max_hp);
		setMovementSpeed(_current_stats.movement_speed);
	}

	void Player::setBaseStats(const Stats& stats) {
		_base_stats = stats;
		recalculateStats();
	}

	void Player::clearItems() {
		if (_backpack)
			_backpack->clear();

		if (_equipment)
			_equipment->clear();

		_food_count = 0;
		updateWeaponVisualModel();
		updatePrimaryAttackAbility();
		recalculateStats();
	}

	void Player::updatePrimaryAttackAbility() {
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

		if (const auto existing = getAbility(PlayerDetail::FIREBALL_ABILITY_SLOT); existing && existing->getName() == "Fireball")
			return;

		const auto icon = _engine->getResourceManager().getTexture(PlayerDetail::FIREBALL_ICON);
		setAbility(PlayerDetail::FIREBALL_ABILITY_SLOT, std::make_shared<FireballAbility>(
			PlayerDetail::FIREBALL_MODEL,
			0.5f,
			nullptr,
			icon,
			&_engine->getResourceManager()));
	}

} // namespace Nawia::Entity
