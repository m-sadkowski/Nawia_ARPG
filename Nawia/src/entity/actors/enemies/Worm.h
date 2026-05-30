#pragma once

#include <SimpleMeleeEnemy.h>

#include <memory>

namespace Nawia::Entity {

	class MiniMushroomInfected;

	class Worm : public SimpleMeleeEnemy {
	public:
		void setLinkedMushroom(const std::weak_ptr<MiniMushroomInfected>& mushroom);

	protected:
		void onDeathStarted() override;

	private:
		Worm();
		friend class WormBuilder;

		std::weak_ptr<MiniMushroomInfected> _linked_mushroom;
	};

	class WormBuilder : public EnemyBuilder<WormBuilder> {
	public:
		WormBuilder() {
			_worm_ptr = std::unique_ptr<Worm>(new Worm());
			this->_entity = _worm_ptr.get();
		}

		WormBuilder& setLinkedMushroom(const std::weak_ptr<MiniMushroomInfected>& mushroom) {
			_worm_ptr->_linked_mushroom = mushroom;
			return *this;
		}

		std::unique_ptr<Worm> build() {
			return std::move(_worm_ptr);
		}

	private:
		std::unique_ptr<Worm> _worm_ptr;
	};

} // namespace Nawia::Entity
