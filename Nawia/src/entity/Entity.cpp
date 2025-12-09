#include "Entity.h"

namespace Nawia::Entity {

	void Entity::render(SDL_Renderer* renderer, float offsetX, float offsetY) {
		if (!_texture) {
			std::cerr << "[ERROR] Entity - Couldn't load texture.\n";
			return;
		}

		float screenX = (_x - _y) * (Core::TILE_WIDTH / 2.0f) + offsetX;
		float screenY = (_x + _y) * (Core::TILE_HEIGHT / 2.0f) + offsetY;

		screenX += (Core::TILE_WIDTH / 2.0f) - (ENTITY_TEXTURE_WIDTH / 2.0f);
		screenY += (ENTITY_TEXTURE_HEIGHT - Core::TILE_HEIGHT);

		SDL_FRect destRect = { screenX, screenY, ENTITY_TEXTURE_WIDTH, ENTITY_TEXTURE_HEIGHT };
		SDL_RenderTexture(renderer, _texture.get(), nullptr, &destRect);
	}

}