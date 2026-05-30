#include "MiniMushroomInfected.h"

#include <SoundIds.h>
#include <Worm.h>

#include <raymath.h>

#include <algorithm>

namespace Nawia::Entity {

	namespace {
		constexpr const char* INFECTED_MODEL = "assets/models/actors/mini_mushroom/mini_mushroom_infected.glb";
		constexpr const char* MINI_MODEL = "assets/models/actors/mini_mushroom/mini_mushroom.glb";
	}

	MiniMushroomInfected::MiniMushroomInfected() {
		setScale(0.3f);
		loadModel(INFECTED_MODEL);
		addAnimation("attack", INFECTED_MODEL, 0);
		addAnimation("death", INFECTED_MODEL, 2);
		addAnimation("get_hit", INFECTED_MODEL, 3);
		addAnimation("idle", INFECTED_MODEL, 4);
		addAnimation("jump", INFECTED_MODEL, 5);
		addAnimation("walk", INFECTED_MODEL, 7);
		configureAnimations("idle", "walk", "attack", "get_hit");
		configureCombat(9.5f, 1.05f, 2.35f, 18, 1.15f, 1.1f, 0.35f);
		playAnimation("idle", true, false, 0, true);
	}

	void MiniMushroomInfected::takeDamage(const int /*dmg*/) {
		if (_corruption_released || _purified)
			return;

		playSoundEffect(Audio::SoundId::MiniMushroomWormExit, 0.8f);
		_corruption_released = true;
		_corpse_frozen = false;
		setType(EntityType::NPCStatic);
		setFaction(Faction::None);
		setVelocity(0.0f, 0.0f);
		_is_moving = false;
		setAnimationSpeed(1.0f);
		playAnimation("death", false, true, 0, true);
		spawnLinkedWorm();
	}

	void MiniMushroomInfected::update(const float dt) {
		if (!_corruption_released) {
			const State previous_state = _state;
			SimpleMeleeEnemy::update(dt);
			updateMovementSound(Audio::SoundPath::MiniMushroomWalk, _state == State::Chasing && _is_moving, 0.34f, 1.35f);
			if (previous_state != State::Attacking && _state == State::Attacking)
				playSoundEffect(Audio::SoundId::MiniMushroomAttack, 0.72f);
			return;
		}

		if (_purified) {
			updatePurifiedProp(dt);
			return;
		}

		Entity::update(dt);
		if (!isAnimationLocked())
			freezeOnDeathFrame();
	}

	void MiniMushroomInfected::spawnLinkedWorm() {
		auto worm = std::shared_ptr<Worm>(WormBuilder()
			.setName("Robal")
			.setPosition(getCenter())
			.setMap(_map)
			.setMaxHp(35)
			.setAudioManager(_audio_manager)
			.setLinkedMushroom(std::dynamic_pointer_cast<MiniMushroomInfected>(shared_from_this()))
			.build());
		worm->setAltitude(getAltitude());
		addPendingSpawn(worm);
	}

	void MiniMushroomInfected::freezeOnDeathFrame() {
		const int frame_count = getAnimationFrameCount("death");
		if (frame_count <= 0)
			return;

		_corpse_frozen = true;
		setAnimationSpeed(0.0f);
		playAnimation("death", false, true, std::max(0, frame_count - 1), true);
	}

	void MiniMushroomInfected::purifyAfterWormDeath() {
		if (_purified)
			return;

		_purified = true;
		_purifying = true;
		_corpse_frozen = false;
		_jump_timer = 1.6f;
		_jumping = false;
		setAnimationSpeed(1.0f);
		loadMiniMushroomAnimations();

		const int frame_count = getAnimationFrameCount("death");
		playAnimation("death", false, true, std::max(0, frame_count - 1), true);
		_anim_direction = -1.0f;
	}

	void MiniMushroomInfected::setPropDestination(const Vector2 destination) {
		if (!_purified)
			return;

		_prop_route.clear();
		_prop_route_index = 0;
		_has_prop_destination = true;
		_prop_destination = destination;
		setMovementSpeed(2.1f);
		moveTo(destination.x, destination.y);
		if (getAnimationFrameCount("walk") > 0)
			playAnimation("walk");
	}

	void MiniMushroomInfected::setPropRoute(const std::vector<Vector2>& route) {
		if (!_purified)
			return;

		_prop_route = route;
		_prop_route_index = 0;
		setMovementSpeed(2.55f);
		moveToNextPropRoutePoint();
	}

	void MiniMushroomInfected::moveToNextPropRoutePoint() {
		while (_prop_route_index < _prop_route.size() &&
			Vector2DistanceSqr(getCenter(), _prop_route[_prop_route_index]) < 0.20f) {
			_prop_route_index++;
		}

		if (_prop_route_index >= _prop_route.size()) {
			_has_prop_destination = false;
			_prop_route.clear();
			_prop_route_index = 0;
			return;
		}

		_has_prop_destination = true;
		_prop_destination = _prop_route[_prop_route_index];
		moveTo(_prop_destination.x, _prop_destination.y);
		if (getAnimationFrameCount("walk") > 0)
			playAnimation("walk");
	}

	void MiniMushroomInfected::loadMiniMushroomAnimations() {
		loadModel(MINI_MODEL);
		addAnimation("death", MINI_MODEL, 2);
		addAnimation("idle", MINI_MODEL, 4);
		addAnimation("jump", MINI_MODEL, 5);
		addAnimation("walk", MINI_MODEL, 7);
	}

	void MiniMushroomInfected::updatePurifiedProp(const float dt) {
		Entity::update(dt);

		if (_purifying) {
			updateMovementSound(Audio::SoundPath::MiniMushroomWalk, false);
			if (!isAnimationLocked()) {
				_purifying = false;
				playAnimation("idle", true, false, 0, true);
			}
			return;
		}

		if (_has_prop_destination) {
			updateMovement(dt);
			updateMovementSound(Audio::SoundPath::MiniMushroomWalk, _is_moving, 0.34f, 1.45f);
			if (_is_moving) {
				if (getAnimationFrameCount("walk") > 0)
					playAnimation("walk");
				return;
			}

			if (!_prop_route.empty()) {
				_prop_route_index++;
				moveToNextPropRoutePoint();
				if (_has_prop_destination)
					return;
			}

			_has_prop_destination = false;
			_jump_timer = static_cast<float>(GetRandomValue(120, 300)) / 100.0f;
			playAnimation("idle", true, false, 0, true);
		}

		updateMovementSound(Audio::SoundPath::MiniMushroomWalk, false);

		if (_jumping) {
			if (!isAnimationLocked()) {
				_jumping = false;
				_jump_timer = static_cast<float>(GetRandomValue(220, 520)) / 100.0f;
				playAnimation("idle");
			}
			return;
		}

		_jump_timer -= dt;
		if (_jump_timer <= 0.0f) {
			_jumping = true;
			playAnimation("jump", false, true, 0, true);
		}
	}

} // namespace Nawia::Entity
