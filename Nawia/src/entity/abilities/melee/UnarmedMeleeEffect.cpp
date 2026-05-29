#include "UnarmedMeleeEffect.h"

#include <Collider.h>
#include <EnemyInterface.h>
#include <Logger.h>
#include <Player.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace Nawia::Entity {

	UnarmedMeleeEffect::UnarmedMeleeEffect(
		const float x,
		const float y,
		const float angle,
		const AbilityStats& stats,
		Entity* caster,
		const Shape shape,
		const float width,
		const float knockback_distance)
		: AbilityEffect("Unarmed Melee", x, y, nullptr, stats),
		  _caster(caster),
		  _shape(shape),
		  _width(width),
		  _knockback_distance(knockback_distance) {
		setRotation(angle);
		if (_shape == Shape::Cone)
			setCollider(std::make_unique<ConeCollider>(this, stats.hitbox_radius, _width));
	}

	void UnarmedMeleeEffect::render(const Camera3D& camera) {
		if (!DebugColliders)
			return;

		if (_collider) {
			_collider->render(camera);
			return;
		}

		if (_shape == Shape::ForwardRectangle) {
			const float rotation = getRotation() * DEG2RAD;
			const Vector2 forward = {std::cos(rotation), std::sin(rotation)};
			const Vector3 center = {
				getCenter().x + forward.x * _stats.hitbox_radius * 0.5f,
				getAltitude() + 0.1f,
				getCenter().y + forward.y * _stats.hitbox_radius * 0.5f
			};
			DrawCubeWires(center, _width, 0.12f, _stats.hitbox_radius, ORANGE);
		}
	}

	bool UnarmedMeleeEffect::checkCollision(const std::shared_ptr<Entity>& target) const {
		if (!_hit_entities.empty())
			return false;

		if (!canHitTarget(target))
			return false;

		const auto enemy = std::dynamic_pointer_cast<EnemyInterface>(target);
		if (!enemy)
			return false;

		if (_shape == Shape::ForwardRectangle) {
			const BoundingBox box = target->getBoundingBox();
			const Vector2 points[] = {
				{0.5f * (box.min.x + box.max.x), 0.5f * (box.min.z + box.max.z)},
				{box.min.x, box.min.z},
				{box.max.x, box.min.z},
				{box.max.x, box.max.z},
				{box.min.x, box.max.z}
			};

			const float rotation = getRotation() * DEG2RAD;
			const Vector2 forward = {std::cos(rotation), std::sin(rotation)};
			const Vector2 right = {-forward.y, forward.x};
			const Vector2 origin = getCenter();

			for (const Vector2 point : points) {
				const Vector2 to_point = {point.x - origin.x, point.y - origin.y};
				const float forward_distance = to_point.x * forward.x + to_point.y * forward.y;
				const float side_distance = to_point.x * right.x + to_point.y * right.y;
				if (forward_distance >= 0.0f && forward_distance <= _stats.hitbox_radius &&
					std::abs(side_distance) <= _width * 0.5f) {
					return true;
				}
			}

			return false;
		}

		if (AbilityEffect::checkCollision(target))
			return true;

		const Vector2 origin = getCenter();
		const Vector2 target_center = enemy->getCenter();
		const Vector2 to_target = {target_center.x - origin.x, target_center.y - origin.y};
		const float distance = std::sqrt(to_target.x * to_target.x + to_target.y * to_target.y);
		if (distance > _stats.hitbox_radius + 0.55f)
			return false;

		const float rotation = getRotation() * DEG2RAD;
		const float target_angle = std::atan2(to_target.y, to_target.x);
		const float angle_diff = std::atan2(std::sin(target_angle - rotation), std::cos(target_angle - rotation));
		const float half_angle = (_width * 0.5f + 10.0f) * DEG2RAD;
		return std::abs(angle_diff) <= half_angle;
	}

	void UnarmedMeleeEffect::onCollision(const std::shared_ptr<Entity>& target) {
		if (!_hit_entities.empty())
			return;

		const auto enemy = std::dynamic_pointer_cast<EnemyInterface>(target);
		if (!enemy)
			return;

		int final_damage = getDamage();
		if (const auto player = dynamic_cast<Player*>(_caster))
			final_damage += std::max(0, player->getStats().damage / 2);

		enemy->takeDamage(final_damage);

		if (_knockback_distance > 0.0f && _caster) {
			const Vector2 caster_center = _caster->getCenter();
			const Vector2 target_center = enemy->getCenter();
			Vector2 direction = {target_center.x - caster_center.x, target_center.y - caster_center.y};
			const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
			if (length > 0.001f) {
				direction.x /= length;
				direction.y /= length;
				enemy->setX(enemy->getX() + direction.x * _knockback_distance);
				enemy->setY(enemy->getY() + direction.y * _knockback_distance);
			}
		}

		addHit(enemy);
		Core::Logger::debugLog(
			getName() + " trafil " + enemy->getName() + " za " + std::to_string(final_damage) + " obrazen.");
	}

} // namespace Nawia::Entity
