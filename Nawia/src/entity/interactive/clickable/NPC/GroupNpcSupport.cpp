#include "GroupNpcSupport.h"

#include <Engine.h>
#include <EntityManager.h>
#include <EntityPathMotion.h>
#include <HerbalistHub.h>
#include <Map.h>

#include <algorithm>
#include <cmath>
#include <raymath.h>

namespace Nawia::Entity {

	GroupNpcVisual::GroupNpcVisual(const std::string& name, const float x, const float y)
		: Entity(name, x, y, nullptr, 1)
	{
		setType(EntityType::NPCStatic);
		setFaction(Faction::None);
	}

	bool GroupNpcVisual::isMovingToTarget() const {
		return isMoving();
	}

	void GroupNpcVisual::updateMoveToTarget(const float delta_time) {
		updateMovement(delta_time);
	}

	void GroupNpcVisual::updateVisualAnimation(const float delta_time) {
		updateAnimation(delta_time);
	}

	void GroupNpcVisual::holdAnimationFrame(const std::string& animation_name, const int frame) {
		Entity::holdAnimationFrame(animation_name, frame);
	}

	void GroupNpcVisual::playAnimationReverseOnce(const std::string& animation_name) {
		Entity::playAnimationReverseOnce(animation_name);
	}

	namespace GroupNpcSupport {

		Vector2 normalizedOrFallback(const Vector2 vector, const Vector2 fallback) {
			if (Vector2LengthSqr(vector) <= 0.0001f)
				return fallback;
			return Vector2Normalize(vector);
		}

		std::optional<GroupNpcHubDestination> resolveHub(
			Core::Engine& engine,
			const std::string& hub_name,
			const float fallback_radius)
		{
			for (const auto& entity : engine.getEntityManager().getEntities()) {
				if (!entity || entity->getName() != hub_name)
					continue;

				GroupNpcHubDestination hub;
				hub.center = {entity->getX(), entity->getY()};
				hub.radius = fallback_radius;
				if (const auto herbalist_hub = dynamic_cast<HerbalistHub*>(entity.get()))
					hub.radius = herbalist_hub->getRadius();
				return hub;
			}

			return std::nullopt;
		}

		void buildPathToPoint(Entity& entity, Core::Engine* engine, const Vector2 target, std::vector<Vector2>& path) {
			PathMotion::buildPathToPoint(
				entity,
				engine ? engine->getCurrentMap() : nullptr,
				target,
				path);
		}

		void trimPathStart(const Entity& entity, std::vector<Vector2>& path) {
			PathMotion::trimPathStart(entity, path);
		}

		void updatePathMovement(Entity& entity, const float delta_time, std::vector<Vector2>& path) {
			PathMotion::updatePathMovement(entity, delta_time, path);
		}

		void stopPathMovement(Entity& entity, std::vector<Vector2>& path) {
			PathMotion::stopPathMovement(entity, path);
		}

		Vector2 randomPointInHub(const GroupNpcHubDestination& hub) {
			const float angle = static_cast<float>(GetRandomValue(0, 6283)) / 1000.0f;
			const float radius = std::sqrt(static_cast<float>(GetRandomValue(0, 10000)) / 10000.0f) *
				std::max(0.1f, hub.radius);
			return {
				hub.center.x + std::cos(angle) * radius,
				hub.center.y + std::sin(angle) * radius
			};
		}

	} // namespace GroupNpcSupport

} // namespace Nawia::Entity
