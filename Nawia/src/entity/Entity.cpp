#include "Entity.h"

#include <Map.h>
#include <Logger.h>

namespace Nawia::Entity {

	Entity::Entity(float start_x, float start_y, const std::shared_ptr<SDL_Texture>& texture, const int max_hp) : _texture(texture), _max_hp(max_hp)
	{
		_pos = std::make_unique<Core::Point2D>(start_x, start_y);

		// set hp
		_hp = _max_hp;
	}

	void Entity::render(SDL_Renderer* renderer, const float offset_x, const float offset_y) {
		if (!_texture) {
			Core::Logger::errorLog("Entity - Could not load texture.");
			return;
		}

		Core::Point2D screen_pos = getScreenPos(_pos->getX(), _pos->getY(), offset_x, offset_y);

		const SDL_FRect dest_rect = { screen_pos.getX(), screen_pos.getY(), Core::ENTITY_TEXTURE_WIDTH, Core::ENTITY_TEXTURE_HEIGHT };
		SDL_RenderTexture(renderer, _texture.get(), nullptr, &dest_rect);
	}

	void Entity::takeDamage(int dmg)
	{
		_hp -= dmg;
		if (_hp < 0) {
			_hp = 0;
			Core::Logger::debugLog("Entity died!");
		}
	}

	bool Entity::isMouseOver(const float mouse_x, const float mouse_y, const float cam_x, const float cam_y) const
	{
		Core::Point2D screen_pos = getScreenPos(mouse_x, mouse_y, cam_x, cam_y);

//		Core::Logger::debugLog("MouseOver Click Raw: " + std::to_string(mouse_x) + ", " + std::to_string(mouse_y));
//		Core::Logger::debugLog("MouseOver Click Screen: " + std::to_string(screen_x) + ", " + std::to_string(screen_y));
//		Core::Logger::debugLog("MouseOver Entity: " + std::to_string(_pos->getX()) + ", " + std::to_string(_pos->getY()));

		return (mouse_x >= screen_pos.getX() && mouse_x <= screen_pos.getX() + Core::ENTITY_TEXTURE_WIDTH &&
			mouse_y >= screen_pos.getY() && mouse_y <= screen_pos.getY() + Core::ENTITY_TEXTURE_HEIGHT);
	}

	Core::Point2D Entity::getScreenPos(const float mouse_x, const float mouse_y, const float cam_x, const float cam_y) const
	{
		SDL_UNUSED(mouse_x);
		SDL_UNUSED(mouse_y);
		float screen_x = (_pos->getX() - _pos->getY()) * (Core::TILE_WIDTH / 2.0f) + cam_x;
		float screen_y = (_pos->getX() + _pos->getY()) * (Core::TILE_HEIGHT / 2.0f) + cam_y;
		screen_x += (Core::TILE_WIDTH / 2.0f) - (Core::ENTITY_TEXTURE_WIDTH / 2.0f);
		screen_y += (Core::ENTITY_TEXTURE_HEIGHT / 2.0f - Core::TILE_HEIGHT * 1.5f);

		return { screen_x, screen_y };
	}
} // namespace Nawia::Entity