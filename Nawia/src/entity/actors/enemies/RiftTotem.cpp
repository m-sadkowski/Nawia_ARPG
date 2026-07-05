#include "RiftTotem.h"

#include <Collider.h>
#include <Map.h>
#include <WalkingDead.h>

#include <raymath.h>

#include <cmath>
#include <memory>
#include <utility>

namespace Nawia::Entity {

	namespace {
		constexpr const char* TOTEM_MODEL = "assets/models/totem.glb";
		constexpr float TOTEM_MODEL_SCALE = 1.0f;
		constexpr const char* HELPER_MODEL = "assets/models/actors/walking_dead/walking_dead_2.glb";
		constexpr float HELPER_MODEL_SCALE = 1.5f;
		constexpr int HELPER_TONGUE_MESH_INDEX = 1;
	}

	RiftTotem::RiftTotem(
		const float x,
		const float y,
		Core::Map* map,
		std::weak_ptr<Entity> owner,
		std::shared_ptr<Entity> target,
		const int stage_index)
		: EnemyInterface("Totem Chaosu", x, y, nullptr, BASE_HP + stage_index * HP_PER_STAGE, map),
		  _owner(std::move(owner)),
		  _stage_index(stage_index),
		  _helper_spawn_timer(0.65f + static_cast<float>(GetRandomValue(0, 90)) / 100.0f)
	{
		setFaction(Faction::Enemy);
		setTarget(target);
		setCollider(std::make_unique<CircleCollider>(this, 0.68f));
		loadModel(TOTEM_MODEL);
		alignLoadedModelToGround();
		setScale(TOTEM_MODEL_SCALE);
	}

	void RiftTotem::update(const float dt)
	{
		const auto owner = _owner.lock();
		if (!owner || owner->isDead() || owner->isDying()) {
			die();
			return;
		}

		if (isDying()) {
			die();
			return;
		}

		if (isDormant())
			return;

		Entity::update(dt);

		if (!_helper_spawned) {
			_helper_spawn_timer -= dt;
			if (_helper_spawn_timer <= 0.0f)
				spawnHelper();
		}
	}

	void RiftTotem::render(const Camera3D& camera)
	{
		if (isDormant() || isDead())
			return;

		renderTether();
		Entity::render(camera);
	}

	void RiftTotem::takeDamage(const int dmg)
	{
		Entity::takeDamage(dmg);
		if (isDying())
			die();
	}

	void RiftTotem::spawnHelper()
	{
		const auto target = getTarget();
		if (!target || target->isDead() || target->isDying())
			return;

		_helper_spawned = true;
		const Vector2 spawn_pos = findHelperSpawnPosition();
		auto helper = WalkingDeadBuilder()
			.setName("Sluga Totemu")
			.setPosition(spawn_pos)
			.setMap(_map)
			.setMaxHp(HELPER_BASE_HP + _stage_index * HELPER_HP_PER_STAGE)
			.setTarget(target)
			.setAudioManager(getAudioManager())
			.build();

		helper->replaceModel(HELPER_MODEL);
		helper->setScale(HELPER_MODEL_SCALE);
		helper->hideMeshIndex(HELPER_TONGUE_MESH_INDEX);

		helper->setAltitude(getAltitude());
		addPendingSpawn(std::shared_ptr<Entity>(std::move(helper)));
	}

	void RiftTotem::renderTether() const
	{
		const auto owner = _owner.lock();
		if (!owner || owner->isDead() || owner->isDying())
			return;

		const Vector3 from = {getCenter().x, getAltitude() + 1.55f, getCenter().y};
		const Vector3 to = {owner->getCenter().x, owner->getAltitude() + 1.25f, owner->getCenter().y};
		DrawLine3D(from, to, Fade(Color{255, 90, 45, 255}, 0.58f));
		DrawLine3D({from.x, from.y + 0.05f, from.z}, {to.x, to.y + 0.05f, to.z}, Fade(Color{255, 210, 80, 255}, 0.35f));
	}

	Vector2 RiftTotem::findHelperSpawnPosition() const
	{
		const Vector2 center = getCenter();
		if (!_map)
			return {center.x + HELPER_SPAWN_RADIUS, center.y};

		const float start_angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
		for (const float radius : {HELPER_SPAWN_RADIUS, HELPER_SPAWN_RADIUS + 0.8f}) {
			for (int i = 0; i < 12; ++i) {
				const float angle = start_angle + (static_cast<float>(i) / 12.0f) * 2.0f * PI;
				const Vector2 candidate = {
					center.x + std::cos(angle) * radius,
					center.y + std::sin(angle) * radius
				};

				if (_map->isWalkable(candidate.x, candidate.y))
					return candidate;
			}
		}

		return center;
	}

} // namespace Nawia::Entity
