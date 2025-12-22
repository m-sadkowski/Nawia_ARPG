#pragma once
#include "AnimationComponent.h"
#include "Entity.h"
#include "Ability.h"

#include <memory>


namespace Nawia::Entity {

	class Player : public Entity {
	public:
		Player(float x, float y, const std::shared_ptr<Texture2D>& texture);

		void update(float delta_time) override;
		void render(float offset_x, float offset_y) override;

		void moveTo(float x, float y);

		// ability system
		void addAbility(const std::shared_ptr<Ability>& ability);
		std::shared_ptr<Ability> getAbility(int index);
		void updateAbilities(float dt) const;

	private:
		float _target_x, _target_y;
		float _speed;
		bool _is_moving;
		std::vector<std::shared_ptr<Ability>> _abilities;
		std::unique_ptr<AnimationComponent> _animation_component;
	};

} // namespace Nawia::Entity