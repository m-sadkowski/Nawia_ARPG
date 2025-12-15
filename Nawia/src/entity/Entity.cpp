#include "Entity.h"

#include <Map.h>

#include <Logger.h>

namespace Nawia::Entity {

	Entity::Entity(float start_x, float start_y, const std::shared_ptr<SDL_Texture>& texture) : _texture(texture)
	{
		_pos = std::make_unique<Core::Point2D>(start_x, start_y);
	}

	void Entity::render(SDL_Renderer* renderer, const float offset_x, const float offset_y) {
		if (!_texture) {
			Core::Logger::errorLog("Entity - Could not load texture.");
			return;
		}

		float screen_x = (_pos->getX() - _pos->getY()) * (Core::TILE_WIDTH / 2.0f) + offset_x;
		float screen_y = (_pos->getX() + _pos->getY()) * (Core::TILE_HEIGHT / 2.0f) + offset_y;

		// offset so the players FEET (BOTTOM OF THE TEXTURE)
		// is rendered exactly where the position is (_x, _y)
		screen_x += (Core::TILE_WIDTH / 2.0f) - (Core::ENTITY_TEXTURE_WIDTH / 2.0f);
		screen_y += (Core::ENTITY_TEXTURE_HEIGHT / 2.0f - Core::TILE_HEIGHT * 1.5f);

		const SDL_FRect dest_rect = { screen_x, screen_y, Core::ENTITY_TEXTURE_WIDTH, Core::ENTITY_TEXTURE_HEIGHT };
		SDL_RenderTexture(renderer, _texture.get(), nullptr, &dest_rect);
	}

} // namespace Nawia::Entity