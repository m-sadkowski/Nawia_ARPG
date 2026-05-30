#pragma once

#include <SimpleMeleeEnemy.h>

#include <memory>
#include <vector>

namespace Nawia::Entity {

	class MiniMushroomInfected : public SimpleMeleeEnemy {
	public:
		void takeDamage(int dmg) override;
		void update(float dt) override;
		void purifyAfterWormDeath();
		[[nodiscard]] bool isPurified() const { return _purified; }
		void setPropDestination(Vector2 destination);
		void setPropRoute(const std::vector<Vector2>& route);

	private:
		void spawnLinkedWorm();
		void freezeOnDeathFrame();
		void updatePurifiedProp(float dt);
		void loadMiniMushroomAnimations();
		void moveToNextPropRoutePoint();

		MiniMushroomInfected();
		friend class MiniMushroomInfectedBuilder;

		bool _corruption_released = false;
		bool _corpse_frozen = false;
		bool _purified = false;
		bool _purifying = false;
		bool _jumping = false;
		bool _has_prop_destination = false;
		std::vector<Vector2> _prop_route;
		size_t _prop_route_index = 0;
		Vector2 _prop_destination = {0.0f, 0.0f};
		float _jump_timer = 2.0f;
	};

	class MiniMushroomInfectedBuilder : public EnemyBuilder<MiniMushroomInfectedBuilder> {
	public:
		MiniMushroomInfectedBuilder() {
			_mushroom_ptr = std::unique_ptr<MiniMushroomInfected>(new MiniMushroomInfected());
			this->_entity = _mushroom_ptr.get();
		}

		std::unique_ptr<MiniMushroomInfected> build() {
			return std::move(_mushroom_ptr);
		}

	private:
		std::unique_ptr<MiniMushroomInfected> _mushroom_ptr;
	};

} // namespace Nawia::Entity
