#pragma once

namespace Nawia::Entity {

	class AllyInterface;

	class AllyBrain {
	public:
		virtual ~AllyBrain() = default;
		virtual void update(AllyInterface& ally, float dt);
	};

} // namespace Nawia::Entity
