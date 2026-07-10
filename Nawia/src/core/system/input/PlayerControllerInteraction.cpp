#include "PlayerController.h"
#include "PlayerControllerInternal.h"

#include <Engine.h>
#include <Entity.h>
#include <Interactable.h>
#include <Player.h>

#include <cmath>

namespace Nawia::Core {

	bool PlayerController::processInteraction() {
		if (!_target_interactable)
			return false;

		const auto target_entity = std::dynamic_pointer_cast<Entity::Entity>(_target_interactable);
		if (!target_entity) {
			_target_interactable = nullptr;
			return false;
		}

		const float interaction_range = std::max(0.0f, _target_interactable->getInteractionRange());
		const float interaction_range_sq = interaction_range * interaction_range;
		const float distance_sq = PlayerControllerDetail::horizontalDistanceToBoxSq(
			*target_entity,
			_player->getCenter()
		);

		if (distance_sq > interaction_range_sq)
			return moveToInteractionRange(target_entity, interaction_range);

		return performInteraction();
	}

	bool PlayerController::moveToInteractionRange(
		const std::shared_ptr<Entity::Entity>& target,
		const float interaction_range
	) {
		if (!_current_path.empty() || _player->isMoving()) {
			updatePathMovement();
		} else if (!moveTowardInteractable(target, interaction_range)) {
			_player->stop();
			_target_interactable = nullptr;
		}
		return true;
	}

	bool PlayerController::performInteraction() {
		_player->stop();
		_target_interactable->onInteract(*_player);
		_target_interactable->onInteractionCompleted(*_player, *_engine);

		_target_interactable = nullptr;
		return true;
	}

	bool PlayerController::moveTowardInteractable(const std::shared_ptr<Entity::Entity>& target, const float interaction_range) {
		if (!target)
			return false;

		const float approach_radius = std::max(0.5f, interaction_range * 0.75f);
		const Vector2 target_center = target->getCenter();
		const Vector3 target_world = target->getWorldPos3D();

		if (buildPathToWorldPosition(target_world)) {
			moveAlongCurrentPath();
			return true;
		}

		constexpr int candidate_count = 12;
		for (int i = 0; i < candidate_count; ++i) {
			const float angle = (static_cast<float>(i) / candidate_count) * 2.0f * PI;
			const Vector3 candidate = {
				target_center.x + std::cos(angle) * approach_radius,
				target_world.y,
				target_center.y + std::sin(angle) * approach_radius
			};

			if (buildPathToWorldPosition(candidate)) {
				moveAlongCurrentPath();
				return true;
			}
		}

		return false;
	}

} // namespace Nawia::Core
