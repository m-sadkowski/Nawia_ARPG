#pragma once

#include <memory>
#include <string>

namespace Nawia::Core {
	class Engine;
}

namespace Nawia::Entity {
	class Entity;
}

namespace Nawia::Game {

	class BossEnemyFactory {
	public:
		[[nodiscard]] static std::shared_ptr<Entity::Entity> create(
			const std::string& type,
			const std::string& name,
			int max_hp,
			Core::Engine* engine);
	};

} // namespace Nawia::Game
