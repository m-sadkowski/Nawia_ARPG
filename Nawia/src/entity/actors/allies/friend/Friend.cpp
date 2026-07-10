#include "Friend.h"

#include <Ability.h>
#include <AllyBrain.h>
#include <Collider.h>
#include <EntityPathMotion.h>
#include <Map.h>
#include <SoundIds.h>

namespace Nawia::Entity {

	Friend::Friend()
	{
		setName("Friend");
		setMaxHp(100);
		setScale(0.015f);
		setMovementSpeed(SPEED);
		setFaction(Faction::Ally);
		setDeathAnimationName("knocked");

		loadModel("assets/models/actors/player/player_idle.glb");
		addAnimation("walk", "assets/models/actors/player/player_walk.glb");
		addAnimation("attack", "assets/models/actors/player/player_auto_attack.glb");
		addAnimation("knocked", "assets/models/actors/player/player_knocked.glb");
		playAnimation("default");

		setCollider(std::make_unique<RectangleCollider>(this, 0.9f, 1.2f, 0.0f, 0.0f));
	}

	void Friend::update(const float dt)
	{
		if (isDying())
		{
			updateMovementSound(Audio::SoundPath::Footsteps, false);
			Entity::update(dt);
			return;
		}

		if (isDormant()) return;

		if (const auto brain = getBrain())
		{
			Entity::update(dt);
			updateAbilities(dt);
			brain->update(*this, dt);
			return;
		}

		updateHardcodedBehavior(dt);
	}

	void Friend::updateHardcodedBehavior(const float dt)
	{
		Entity::update(dt);
		updateAbilities(dt);

		const auto target = getTarget();
		if (!target || target->isDead() || target->isDying())
		{
			stopPathMovement();
			updateMovementSound(Audio::SoundPath::Footsteps, false);
			if (!isAnimationLocked())
				playAnimation("default");
			return;
		}

		if (isAnimationLocked())
		{
			faceTargetCenter();
			return;
		}

		const float dist = getDistanceToTarget();
		if (dist > VISION_RANGE)
		{
			stopPathMovement();
			updateMovementSound(Audio::SoundPath::Footsteps, false);
			playAnimation("default");
			return;
		}

		if (const auto slash = getAbility(0))
		{
			const float attack_range = slash->getCastRange() * ATTACK_RANGE_MULTIPLIER;
			if (slash->isReady() && dist <= attack_range)
			{
				const Vector2 target_pos = target->getCenter();
				rotateTowardsCenter(target_pos.x, target_pos.y);
				slash->cast(target_pos.x, target_pos.y);
				stopPathMovement();
				return;
			}
		}

		tickPathRecalcTimer(dt);
		if (isPathRecalcDue() || (_current_path.empty() && !isMoving()))
		{
			rebuildPathToTarget(*target);
			resetPathRecalcTimer(PATH_RECALC_INTERVAL);
		}

		updatePathMovement(dt);
		updateMovementSound(Audio::SoundPath::Footsteps, isMoving() && !isAnimationLocked(), 0.42f, 1.04f);

		if (isMoving())
			playAnimation("walk");
		else
			playAnimation("default");
	}

	void Friend::stopPathMovement()
	{
		PathMotion::stopPathMovement(*this, _current_path);
	}

	void Friend::rebuildPathToTarget(const Entity& target)
	{
		PathMotion::buildPathToEntity(*this, _map, target, _current_path);
	}

	void Friend::updatePathMovement(const float dt)
	{
		PathMotion::updatePathMovement(*this, dt, _current_path);
	}

	void Friend::onDeathStarted()
	{
		stopPathMovement();
		updateMovementSound(Audio::SoundPath::Footsteps, false);
		playSoundEffect(Audio::SoundId::HumanDeath, 0.85f);
	}

} // namespace Nawia::Entity

