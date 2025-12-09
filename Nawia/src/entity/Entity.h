#pragma once
#include <SDL3/SDL.h>
#include <memory>
#include <iostream>
#include <string>
#include <ResourceManager.h>
#include <Map.h>

namespace Nawia::Entity {

	constexpr int ENTITY_TEXTURE_WIDTH = 128;
	constexpr int ENTITY_TEXTURE_HEIGHT = 64;

	class Entity {
	public:
		Entity(float x, float y, std::shared_ptr<SDL_Texture> texture)
			: _x(x), _y(y), _texture(texture) {}

		virtual ~Entity() = default;

		virtual void update(float deltaTime) = 0;

		virtual void render(SDL_Renderer* renderer, float offsetX, float offsetY);

		float getX() const { return _x; }
		float getY() const { return _y; }

	protected:
		// position in game
		float _x, _y;
		std::shared_ptr<SDL_Texture> _texture;


	};

}