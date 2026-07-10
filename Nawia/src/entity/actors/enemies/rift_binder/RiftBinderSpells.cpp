#include "RiftBinder.h"
#include "RiftBinderInternal.h"

#include <BossTelegraphHazard.h>
#include <FireRainHazard.h>
#include <Projectile.h>
#include <SoundIds.h>

#include <cmath>
#include <memory>
#include <utility>

namespace Nawia::Entity {

	using namespace RiftBinderDetail;

	void RiftBinder::spawnStoneVolley()
	{
		const auto target = getTarget();
		const Vector2 boss_pos = getCenter();
		const Vector2 target_pos = targetCenterOrSelf();
		const float target_height = target ? target->getAltitude() + 0.75f : getAltitude() + 0.75f;
		const Vector2 forward = safeNormalize(Vector2Subtract(target_pos, boss_pos), {1.0f, 0.0f});
		const Vector2 right = {-forward.y, forward.x};

		AbilityStats stats;
		stats.damage = static_cast<int>(std::round(static_cast<float>(STONE_DAMAGE) * getDamageMultiplier()));
		stats.duration = STONE_PROJECTILE_DURATION;
		stats.projectile_speed = STONE_PROJECTILE_SPEED;
		stats.hitbox_radius = STONE_PROJECTILE_HIT_RADIUS;

		for (int i = 0; i < STONE_PROJECTILE_COUNT; ++i) {
			const float side = i == 0 ? 0.0f : (i == 1 ? -1.0f : 1.0f);
			const Vector2 spawn_pos = {
				boss_pos.x + forward.x * STONE_PROJECTILE_SPAWN_FORWARD + right.x * side * STONE_PROJECTILE_SPAWN_SIDE,
				boss_pos.y + forward.y * STONE_PROJECTILE_SPAWN_FORWARD + right.y * side * STONE_PROJECTILE_SPAWN_SIDE
			};
			const Vector2 aim_pos = {
				target_pos.x + right.x * side * STONE_PROJECTILE_SPREAD,
				target_pos.y + right.y * side * STONE_PROJECTILE_SPREAD
			};

			auto projectile = std::make_shared<Projectile>(
				"Stone Shard",
				spawn_pos.x,
				spawn_pos.y,
				aim_pos.x,
				aim_pos.y,
				STONE_PROJECTILE_MODEL,
				STONE_PROJECTILE_MODEL_SCALE,
				stats,
				this,
				target_height);
			projectile->setFaction(Faction::Enemy);
			projectile->setAltitude(getAltitude());
			projectile->setModelTint(STONE_PROJECTILE_TINT);
			addPendingSpawn(projectile);
		}
	}

	void RiftBinder::spawnBlinkFlare(const Vector2 position)
	{
		BossTelegraphHazardConfig config;
		config.name = getName() + " Dragon Flare";
		config.position = findWalkableNearby(position, position);
		config.altitude = getAltitude();
		config.radius = BLINK_FLARE_RADIUS;
		config.warning_seconds = 0.0f;
		config.active_seconds = 0.26f;
		config.damage_per_tick = 0;
		config.tick_interval = 8.0f;
		config.source_context = Entity::makeDamageSourceContext(this, config.name);
		config.warning_color = {155, 80, 255, 255};
		config.active_color = {120, 35, 230, 255};
		addPendingSpawn(std::make_shared<BossTelegraphHazard>(std::move(config)));
	}

	void RiftBinder::spawnFireRain(const float warning_seconds)
	{
		BossTelegraphHazardConfig config;
		config.name = getName() + " Fire Rain";
		config.position = findWalkableNearby(targetCenterOrSelf(), getCenter());
		config.altitude = getAltitude();
		config.radius = FIRE_RAIN_RADIUS + (_shield_active ? 0.25f * static_cast<float>(std::max(0, _active_stage)) : 0.0f);
		config.warning_seconds = warning_seconds;
		config.active_seconds = FIRE_RAIN_ACTIVE_SECONDS;
		config.damage_per_tick = FIRE_RAIN_DAMAGE;
		config.tick_interval = FIRE_RAIN_TICK_INTERVAL;
		config.source_context = Entity::makeDamageSourceContext(this, config.name);
		config.warning_color = {255, 165, 45, 255};
		config.active_color = {245, 58, 18, 255};
		addPendingSpawn(std::make_shared<FireRainHazard>(std::move(config)));
	}

	void RiftBinder::performRandomTeleport()
	{
		const Vector2 old_pos = getCenter();
		const Vector2 destination = findTeleportDestination();
		setX(destination.x);
		setY(destination.y);
		spawnBlinkFlare(old_pos);
		spawnBlinkFlare(destination);
		playSoundEffect(Audio::SoundId::DevilDash, 0.58f, true, 1.12f);
	}

	void RiftBinder::applyMeleeDamage()
	{
		const auto target = getTarget();
		if (target && !target->isDead() && !target->isDying() && getDistanceToTarget() <= MELEE_RANGE * 1.55f) {
			target->takeDamage(
				static_cast<int>(static_cast<float>(MELEE_DAMAGE) * getDamageMultiplier()),
				Entity::makeDamageSourceContext(this, "Dragon Claw"));
			target->applyRoot(MELEE_ROOT_SECONDS);
			playSoundEffect(Audio::SoundId::DevilPunch, 0.72f, true, 0.9f);
		}

		_melee_damage_applied = true;
	}

} // namespace Nawia::Entity
