#include "SwordSlashEffect.h"
#include "Constants.h"
#include "Enemy.h"
#include <Logger.h>
#include <algorithm>
#include <cmath>


namespace Nawia::Entity {

	SwordSlashEffect::SwordSlashEffect(const float x, const float y, const float angle, const std::shared_ptr<SDL_Texture> &tex, const int damage) 
		: AbilityEffect(x, y, tex, 0.2f, damage), _angle(angle) {}

	void SwordSlashEffect::update(const float dt)
	{
		AbilityEffect::update(dt);
	}

	// we override render from Entity beacuse we want to spawn sword rotated
	void SwordSlashEffect::render(SDL_Renderer *renderer, const float camera_x, const float camera_y) 
	{
	  if (!_texture) 
	  {
	    Core::Logger::errorLog("SwordSlashEffect - Could not load texture.");
	    return;
	  }

	  Core::Point2D screen_pos = getScreenPos(_pos->getX(), _pos->getY(), camera_x, camera_y);
	  const SDL_FRect dest_rect = {screen_pos.getX(), screen_pos.getY(), Core::ENTITY_TEXTURE_WIDTH, Core::ENTITY_TEXTURE_HEIGHT};
	  constexpr SDL_FPoint center = {Core::ENTITY_TEXTURE_WIDTH / 4.0f, Core::ENTITY_TEXTURE_HEIGHT / 4.0f};

	  SDL_RenderTextureRotated(renderer, _texture.get(), nullptr, &dest_rect, _angle + 70.0f, &center, SDL_FLIP_NONE);
	}

	bool SwordSlashEffect::checkCollision(const std::shared_ptr<Entity>& target) const
	{
		if (const auto enemy = std::dynamic_pointer_cast<Enemy>(target))
	  {
	    if (enemy->isDead())
	      return false;
		if (hasHit(enemy))
			return false;

		const float dx = enemy->getX() - getX();
		const float dy = enemy->getY() - getY();
		const float distance_squared = dx * dx + dy * dy;

		// range check
		if (distance_squared < 2.5f * 2.5f)
		{
			const float angle_to_enemy = static_cast<float>(std::atan2(dy, dx) * 180.0f / Core::pi);
			const float slash_angle = getAngle();
			float angle_diff = angle_to_enemy - slash_angle;
			while (angle_diff > 180.0f) angle_diff -= 360.0f;
			while (angle_diff < -180.0f) angle_diff += 360.0f;

			if (std::abs(angle_diff) < 45.0f)
				return true;
		}
		}
		return false;
	}

	void SwordSlashEffect::onCollision(const std::shared_ptr<Entity>& target)
	{
		if (const auto enemy = std::dynamic_pointer_cast<Enemy>(target))
		{
			enemy->takeDamage(getDamage());
			addHit(enemy);
			Core::Logger::debugLog("Sword Slash Hit Enemy!");
		}
	}

} // namespace Nawia::Entity
