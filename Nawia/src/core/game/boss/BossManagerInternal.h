#pragma once

#include <Entity.h>

#include <memory>

namespace Nawia::Game::BossManagerDetail {

	inline void applyConfiguredScale(const std::shared_ptr<Entity::Entity>& entity, const float scale) {
		if (entity && scale > 0.0f)
			entity->setScale(scale);
	}

} // namespace Nawia::Game::BossManagerDetail
