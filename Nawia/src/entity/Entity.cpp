#include "Entity.h"

namespace Nawia::Entity {

	void Entity::render(SDL_Renderer* renderer, float offsetX, float offsetY) {
		if (!_texture) {
			std::cerr << "[ERROR] Entity - Couldn't load texture.\n";
			return;
		}

		float screenX = (_pos->getX() - _pos->getY()) * (Core::TILE_WIDTH / 2.0f) + offsetX;
		float screenY = (_pos->getX() + _pos->getY()) * (Core::TILE_HEIGHT / 2.0f) + offsetY;

		// offset so the players FEET (BOTTOM OF THE TEXTURE)
		// is rendered exactly where the position is (_x, _y)
		screenX += (Core::TILE_WIDTH / 2.0f) - (ENTITY_TEXTURE_WIDTH);
		screenY += (ENTITY_TEXTURE_HEIGHT / 2.0f - Core::TILE_HEIGHT * 1.5f);

		SDL_FRect destRect = { screenX, screenY, ENTITY_TEXTURE_WIDTH, ENTITY_TEXTURE_HEIGHT };
		SDL_RenderTexture(renderer, _texture.get(), nullptr, &destRect);
	}

}