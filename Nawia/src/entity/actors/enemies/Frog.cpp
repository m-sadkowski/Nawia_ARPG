#include "Frog.h"

#include <Map.h>
#include <SoundIds.h>
#include <VillageHeadNpc.h>

#include <raymath.h>

#include <algorithm>
#include <cmath>
#include <memory>

namespace Nawia::Entity {

	namespace {
		constexpr const char* MODEL_PATH = "assets/models/actors/frog/frog.glb";
	}

	Frog::Frog() {
		setScale(1.5f);
		loadModel(MODEL_PATH);
		addAnimation("attack", MODEL_PATH, 0);
		addAnimation("death", MODEL_PATH, 1);
		addAnimation("idle", MODEL_PATH, 2);
		addAnimation("walk", MODEL_PATH, 3);
		configureAnimations("idle", "walk", "attack");
		configureCombat(18.0f, 3.8f, 4.2f, 20, 0.8f, 2.15f, 0.32f);
		playAnimation("idle", true, false, 0, true);
	}

	void Frog::update(const float dt) {
		if (_retreat_timer > 0.0f && !isDying()) {
			_retreat_timer -= dt;

			setSpeedMultiplier(1.75f);
			if (!isAnimationLocked() && getAnimationFrameCount("walk") > 0)
				playAnimation("walk");

			Entity::update(dt);

			if (hasValidTarget()) {
				const auto target = getTarget();
				Vector2 away = {
					getCenter().x - target->getCenter().x,
					getCenter().y - target->getCenter().y
				};
				const float length = std::sqrt(away.x * away.x + away.y * away.y);
				if (length > 0.001f) {
					away.x /= length;
					away.y /= length;
				} else {
					away = {1.0f, 0.0f};
				}

				Vector3 retreat_position = {
					getX() + away.x * 5.0f,
					getAltitude(),
					getY() + away.y * 5.0f
				};
				if (_map && _map->getNavMesh().isReady())
					retreat_position = _map->getNavMesh().getClosestWalkablePosition(retreat_position);

				moveTowardPositionWithNav({retreat_position.x, retreat_position.z}, dt);
			} else {
				clearNavigationPath();
				_is_moving = false;
			}

			updateMovementSound(Audio::SoundPath::DevilStep, _is_moving && !isDormant(), 0.32f, 1.25f);
			if (_retreat_timer <= 0.0f || !_is_moving) {
				_retreat_timer = 0.0f;
				clearNavigationPath();
				_attack_cooldown_timer = std::max(_attack_cooldown_timer, 1.0f);
				_state = State::Chasing;
				setAnimationSpeed(1.0f);
			}
			return;
		}

		if (hasValidTarget() && getDistanceToTarget() > 8.0f)
			setSpeedMultiplier(1.65f);
		else
			setSpeedMultiplier(1.0f);

		const State previous_state = _state;
		SimpleMeleeEnemy::update(dt);
		updateMovementSound(Audio::SoundPath::DevilStep, _is_moving && !isDying() && !isDormant(), 0.30f, 1.25f);

		if (previous_state == State::Idle && _state == State::Chasing)
			playSoundEffect(Audio::SoundId::DevilAggro, 0.72f, true, 0.82f);
	}

	void Frog::onAttackDamageApplied(Entity& target) {
		playSoundEffect(Audio::SoundId::DevilPunch, 0.82f, true, 1.15f);
		_retreat_timer = 1.25f;
		clearNavigationPath();
		_attack_cooldown_timer = std::max(_attack_cooldown_timer, 1.15f);
		rotateTowardsCenter(target.getCenter().x, target.getCenter().y);
	}

	void Frog::onDeathStarted() {
		playSoundEffect(Audio::SoundId::DevilDeath, 0.85f, true, 0.9f);

		if (!_engine)
			return;

		auto village_head = std::make_shared<VillageHeadNpc>("Soltys", getX(), getY(), _engine);
		village_head->setAltitude(getAltitude());
		village_head->setAudioManager(_audio_manager);
		addPendingSpawn(village_head);
	}

} // namespace Nawia::Entity
