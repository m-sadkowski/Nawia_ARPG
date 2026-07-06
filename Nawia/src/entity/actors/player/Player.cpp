#include "Player.h"
#include "PlayerInternal.h"

#include <Engine.h>

namespace Nawia::Entity {

	Player::Player() {
		setName("Player");
		setMaxHp(200);
		setScale(1.5f);
		setType(EntityType::Player);
		setFaction(Faction::Player);
		_active_visual_model_path = PlayerDetail::PLAYER_HEAD_MODEL;
		loadModel(_active_visual_model_path);
		loadAnimationBundle("assets/models/animations/anims.glb");
		loadAnimationBundle("assets/models/animations/anims2.glb");
		playAnimation("Idle_Loop");
		setAnimationSpeed(1.0f);
		setDeathAnimationName("Death01");

		_backpack = std::make_unique<Item::Backpack>(INIT_BACKPACK_SIZE);

		_base_stats.max_hp = getMaxHP();
		_base_stats.damage = 10;
		_base_stats.attack_speed = 1.0f;
		_base_stats.movement_speed = 4.0f;
		_base_stats.defense = 0;

		_current_stats = _base_stats;
		setMovementSpeed(_current_stats.movement_speed);
	}

	void Player::attachEngine(Core::Engine* engine) {
		_engine = engine;
		if (!_engine) return;

		_equipment = std::make_unique<Item::Equipment>(_engine->getResourceManager());
		updateWeaponVisualModel();
		recalculateStats();
	}

	void Player::updateAttachedModelAnimation(const ModelAnimation& animation, const int frame) {
		if (_equipment)
			_equipment->updateAnimations(animation, frame);
	}

	void Player::drawAttachedModel(const Vector3 pos3d, const float visual_rotation) const {
		if (_equipment)
			_equipment->draw(pos3d, visual_rotation, getRotation(), getScale());
	}

} // namespace Nawia::Entity
