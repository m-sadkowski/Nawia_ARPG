#include "RiftBinder.h"
#include "RiftBinderInternal.h"

#include <Collider.h>
#include <SoundIds.h>

#include <algorithm>
#include <memory>

namespace Nawia::Entity {

	using namespace RiftBinderDetail;

	RiftBinder::RiftBinder()
	{
		setName("Siewca Chaosu");
		setMaxHp(420);
		setFaction(Faction::Enemy);
		setMovementSpeed(MOVE_SPEED);
		setCollider(std::make_unique<RectangleCollider>(this, 1.25f, 1.55f, 0.0f, 0.0f));
		configureModel();
	}

	RiftBinder::RiftBinder(const float x, const float y, Core::Map* map)
		: EnemyInterface("Siewca Chaosu", x, y, nullptr, 420, map)
	{
		setFaction(Faction::Enemy);
		setMovementSpeed(MOVE_SPEED);
		setCollider(std::make_unique<RectangleCollider>(this, 1.25f, 1.55f, 0.0f, 0.0f));
		configureModel();
	}

	void RiftBinder::configureModel()
	{
		loadModel(DRAGON_MODEL);
		if (hasModelLoaded())
			fitLoadedModelToHeight(DRAGON_TARGET_HEIGHT);
		addAnimation("death", DRAGON_MODEL, DRAGON_ANIM_DEATH);
		addAnimation("walk", DRAGON_MODEL, DRAGON_ANIM_FAST_FLYING);
		addAnimation("run", DRAGON_MODEL, DRAGON_ANIM_FAST_FLYING);
		addAnimation("idle", DRAGON_MODEL, DRAGON_ANIM_FLYING_IDLE);
		addAnimation("attack", DRAGON_MODEL, DRAGON_ANIM_HEADBUTT);
		addAnimation("fire_rain", DRAGON_MODEL, DRAGON_ANIM_HEADBUTT);
		addAnimation("get_hit", DRAGON_MODEL, DRAGON_ANIM_HIT_REACT);
		addAnimation("teleport", DRAGON_MODEL, DRAGON_ANIM_NO);
		addAnimation("stone_volley", DRAGON_MODEL, DRAGON_ANIM_PUNCH);
		playIdle();
	}

	void RiftBinder::update(const float dt)
	{
		updateMovementSound(Audio::SoundPath::DevilStep, isMoving() && !isDying() && !isDormant(), 0.38f, 0.75f);

		if (isDying()) {
			Entity::update(dt);
			return;
		}

		if (isDormant())
			return;

		updateTotemStage();

		if (_action_cooldown_timer > 0.0f)
			_action_cooldown_timer -= dt;
		if (_stone_cooldown_timer > 0.0f)
			_stone_cooldown_timer -= dt;
		if (_fire_rain_cooldown_timer > 0.0f)
			_fire_rain_cooldown_timer -= dt;
		if (_blink_cooldown_timer > 0.0f)
			_blink_cooldown_timer -= dt;
		if (_melee_cooldown_timer > 0.0f)
			_melee_cooldown_timer -= dt;

		if (canCastFireRain())
			_fire_rain_cooldown_timer = std::min(_fire_rain_cooldown_timer, currentFireRainCooldown());

		switch (_state) {
			case State::Idle:
				handleIdleState(dt);
				break;
			case State::Repositioning:
				handleRepositioningState(dt);
				break;
			case State::Casting:
				handleCastingState(dt);
				break;
			case State::Recovering:
				handleRecoveringState(dt);
				break;
			case State::MeleeAttacking:
				handleMeleeAttackingState(dt);
				break;
			case State::HitReacting:
				handleHitReactingState(dt);
				break;
		}
	}

	void RiftBinder::takeDamage(const int dmg)
	{
		if (dmg <= 0) {
			Entity::takeDamage(dmg);
			return;
		}

		if (_next_stage_to_start == 0) {
			startTotemStage(0);
			return;
		}

		if (_shield_active) {
			if (livingTotemCount() > 0) {
				playSoundEffect(Audio::SoundId::DevilPunch, 0.28f, true, 0.65f);
				return;
			}
			_shield_active = false;
			_active_stage = -1;
		}

		if (_next_stage_to_start > 0 && _next_stage_to_start < static_cast<int>(TOTEMS_BY_STAGE.size())) {
			const int threshold_hp = stageThresholdHp(getMaxHP(), _next_stage_to_start);
			if (getHP() > threshold_hp && getHP() - dmg <= threshold_hp) {
				const int damage_to_threshold = std::max(0, getHP() - threshold_hp);
				if (damage_to_threshold > 0)
					Entity::takeDamage(damage_to_threshold);

				if (!isDying() && !isDead())
					startTotemStage(_next_stage_to_start);
				return;
			}
		}

		Entity::takeDamage(dmg);
		startHitReact();
	}

	Vector3 RiftBinder::getWorldPos3D() const
	{
		return {getX(), getAltitude() + DRAGON_VISUAL_HEIGHT_OFFSET, getY()};
	}

	bool RiftBinder::isVisibleInCamera(const Camera3D& camera, const float screen_margin) const
	{
		(void)camera;
		(void)screen_margin;
		return !isDormant() && !isDead();
	}

} // namespace Nawia::Entity
