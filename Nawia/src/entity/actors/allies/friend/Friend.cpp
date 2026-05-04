#include "Friend.h"

#include <Ability.h>
#include <AllyBrain.h>
#include <Collider.h>

namespace Nawia::Entity {

	Friend::Friend()
	{
		setName("Friend");
		setMaxHp(100);
		setScale(0.015f);
		setMovementSpeed(SPEED);
		setFaction(Faction::Ally);
		_death_anim_name = "knocked";

		loadModel("../assets/models/player_idle.glb");
		addAnimation("walk", "../assets/models/player_walk.glb");
		addAnimation("attack", "../assets/models/player_auto_attack.glb");
		addAnimation("knocked", "../assets/models/player_knocked.glb");
		playAnimation("default");

		setCollider(std::make_unique<RectangleCollider>(this, 0.9f, 1.2f, 0.0f, 0.0f));
	}

	void Friend::update(const float dt)
	{
		if (isDying())
		{
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

		const auto target = _target.lock();
		if (!target || target->isDead() || target->isDying())
		{
			_is_moving = false;
			if (!isAnimationLocked())
				playAnimation("default");
			return;
		}

		if (isAnimationLocked())
		{
			rotateTowardsCenter(target->getCenter().x, target->getCenter().y);
			return;
		}

		const float dist = getDistanceToTarget();
		if (dist > VISION_RANGE)
		{
			_is_moving = false;
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
				_is_moving = false;
				return;
			}
		}

		const Vector2 target_pos = target->getCenter();
		moveTo(target_pos.x, target_pos.y);
		updateMovement(dt);

		if (_is_moving)
			playAnimation("walk");
		else
			playAnimation("default");
	}

} // namespace Nawia::Entity

