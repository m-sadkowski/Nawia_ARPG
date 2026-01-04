#pragma once
#include "Entity.h"
#include "Ability.h"

#include <memory>
#include <vector>

namespace Nawia::Entity {

	class Player : public Entity {
	public:
		Player(float x, float y, const std::shared_ptr<Texture2D>& texture);

		void update(float delta_time) override;
		[[nodiscard]] bool isMoving() const { return _is_moving; }

		void moveTo(float x, float y);
		void stop();
		void updateMovement(float delta_time);

	private:
		float _target_x, _target_y;
		float _speed;
		bool _is_moving;
	};

} // namespace Nawia::Entity