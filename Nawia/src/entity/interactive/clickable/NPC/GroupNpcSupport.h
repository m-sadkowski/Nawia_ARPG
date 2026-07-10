#pragma once

#include <Entity.h>

#include <optional>
#include <string>
#include <vector>

namespace Nawia::Core {
	class Engine;
}

namespace Nawia::Entity {

	struct GroupNpcHubDestination {
		Vector2 center = {0.0f, 0.0f};
		float radius = 5.0f;
	};

	class GroupNpcVisual final : public Entity {
	public:
		GroupNpcVisual(const std::string& name, float x, float y);

		[[nodiscard]] bool isMovingToTarget() const;
		void updateMoveToTarget(float delta_time);
		void updateVisualAnimation(float delta_time);
		void holdAnimationFrame(const std::string& animation_name, int frame);
		void playAnimationReverseOnce(const std::string& animation_name);
	};

	namespace GroupNpcSupport {
		[[nodiscard]] Vector2 normalizedOrFallback(Vector2 vector, Vector2 fallback);
		[[nodiscard]] std::optional<GroupNpcHubDestination> resolveHub(
			Core::Engine& engine,
			const std::string& hub_name,
			float fallback_radius);
		void buildPathToPoint(Entity& entity, Core::Engine* engine, Vector2 target, std::vector<Vector2>& path);
		void updatePathMovement(Entity& entity, float delta_time, std::vector<Vector2>& path);
		void stopPathMovement(Entity& entity, std::vector<Vector2>& path);
		[[nodiscard]] Vector2 randomPointInHub(const GroupNpcHubDestination& hub);
	}

} // namespace Nawia::Entity
