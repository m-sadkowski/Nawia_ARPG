#pragma once

#include <SimpleMeleeEnemy.h>

#include <memory>

namespace Nawia::Entity {

	class MiniMushroomInfected;

	class Worm : public SimpleMeleeEnemy {
	public:
		Worm();

		void setLinkedMushroom(const std::weak_ptr<MiniMushroomInfected>& mushroom);

	protected:
		void onDeathStarted() override;

	private:
		std::weak_ptr<MiniMushroomInfected> _linked_mushroom;
	};

} // namespace Nawia::Entity
