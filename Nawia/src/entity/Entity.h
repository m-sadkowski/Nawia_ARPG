#pragma once
#include <SDL3/SDL.h>
#include <memory>
#include <iostream>
#include <string>
#include <ResourceManager.h>
#include <MathUtils.h>


namespace Nawia::Entity {

	constexpr int ENTITY_TEXTURE_WIDTH = 128;
	constexpr int ENTITY_TEXTURE_HEIGHT = 64;

	class Entity {
	public:
		Entity(float startX, float startY, std::shared_ptr<SDL_Texture> texture)
			: _texture(texture) {
			_pos = std::make_unique<Core::Point2D>(startX, startY);
		}

		virtual ~Entity() = default;

		virtual void update(float deltaTime) = 0;

		virtual void render(SDL_Renderer* renderer, float offsetX, float offsetY);
	protected:
		// position in game - uses Point2D
		// while creating an entity, pass (x, y)
		// and it will be converted to a Point2D assigned to said entity
		// to get/set values use _pos->getX/Y or _pos->setX/Y
		std::unique_ptr<Core::Point2D> _pos;
		std::shared_ptr<SDL_Texture> _texture;
	};

}