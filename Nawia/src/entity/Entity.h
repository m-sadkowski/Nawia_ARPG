#pragma once
#include "MathUtils.h"
#include "Constants.h"

#include <SDL3/SDL.h>
#include <memory>
#include <iostream>

namespace Nawia::Entity {

	class Entity {
	public:
		Entity(float start_x, float start_y, const std::shared_ptr<SDL_Texture>& texture);
		virtual ~Entity() = default;

		virtual void update(float delta_time) = 0;
		virtual void render(SDL_Renderer* renderer, float offset_x, float offset_y);

		float getX() { return _pos->getX(); }
		float getY() { return _pos->getY(); }

	protected:
		// position in game - uses Point2D
		// while creating an entity, pass (x, y)
		// and it will be converted to a Point2D assigned to said entity
		// to get/set values use _pos->getX/Y or _pos->setX/Y
		std::unique_ptr<Core::Point2D> _pos;
		std::shared_ptr<SDL_Texture> _texture;
	};

} // namespace Nawia::Entity