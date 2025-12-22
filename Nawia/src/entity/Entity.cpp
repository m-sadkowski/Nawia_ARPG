#include "Entity.h"

#include <Logger.h>
#include <Map.h>


namespace Nawia::Entity {

	Entity::Entity(float start_x, float start_y, const std::shared_ptr<Texture2D>& texture, const int max_hp) 
		: _texture(texture), _max_hp(max_hp), _hp(max_hp) 
	{	
		_pos = std::make_unique<Core::Point2D>(start_x, start_y);
	}

	void Entity::render(const float offset_x, const float offset_y) 
	{
		if (!_texture) 
		{
			Core::Logger::errorLog("Entity - could not load texture.");
			return;
		}

		Core::Point2D screen_pos = getScreenPos(_pos->getX(), _pos->getY(), offset_x, offset_y);

		// Renders the entity to the screen relative to the camera offset, automatically scaling the sprite to the defined target dimensions.
		const float source_texture_width = static_cast<float>(_texture->width);
		const float source_texture_height = static_cast<float>(_texture->height);
		constexpr float dest_texture_width = static_cast<float>(Core::ENTITY_TEXTURE_WIDTH);
		constexpr float dest_texture_height = static_cast<float>(Core::ENTITY_TEXTURE_HEIGHT);

		const Rectangle source = {0.0f, 0.0f, source_texture_width, source_texture_height };
		const Rectangle dest = {screen_pos.getX(), screen_pos.getY(), dest_texture_width, dest_texture_height };
		constexpr Vector2 origin = {0.0f, 0.0f};
		DrawTexturePro(*_texture, source, dest, origin, 0.0f, WHITE);
	}

	void Entity::takeDamage(const int dmg) 
	{
		_hp -= dmg;
		if (_hp < 0) 
		{
			_hp = 0;
			Core::Logger::debugLog("Entity died!");
		}
	}

	void Entity::die()
	{
		_hp = 0;
		Core::Logger::debugLog("Entity killed!");
	}

	bool Entity::isMouseOver(const float mouse_x, const float mouse_y, const float cam_x, const float cam_y) const 
	{
		Core::Point2D screen_pos = getScreenPos(mouse_x, mouse_y, cam_x, cam_y);

		// Core::Logger::debugLog("MouseOver click raw: " + std::to_string(mouse_x) + ", " + std::to_string(mouse_y));
		// Core::Logger::debugLog("MouseOver click screen: " + std::to_string(screen_x) + ", " + std::to_string(screen_y));
		// Core::Logger::debugLog("MouseOver Entity: " + std::to_string(_pos->getX()) + ", " + std::to_string(_pos->getY()));

		return (mouse_x >= screen_pos.getX() &&  mouse_x <= screen_pos.getX() + Core::ENTITY_TEXTURE_WIDTH &&
		      mouse_y >= screen_pos.getY() && mouse_y <= screen_pos.getY() + Core::ENTITY_TEXTURE_HEIGHT);
	}

	Core::Point2D Entity::getScreenPos(const float mouse_x, const float mouse_y, const float cam_x, const float cam_y) const 
	{
		// unused
		(void)mouse_x;
		(void)mouse_y; 

		float screen_x = (_pos->getX() - _pos->getY()) * (Core::TILE_WIDTH / 2.0f) + cam_x;
		float screen_y = (_pos->getX() + _pos->getY()) * (Core::TILE_HEIGHT / 2.0f) + cam_y;
		screen_x += (Core::TILE_WIDTH / 2.0f) - (Core::ENTITY_TEXTURE_WIDTH / 2.0f);
		screen_y += (Core::ENTITY_TEXTURE_HEIGHT / 2.0f - Core::TILE_HEIGHT * 1.5f);

		return {screen_x, screen_y};
	}

} // namespace Nawia::Entity