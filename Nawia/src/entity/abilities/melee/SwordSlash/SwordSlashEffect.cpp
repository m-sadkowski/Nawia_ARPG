#include "SwordSlashEffect.h"
#include "EnemyInterface.h"

#include <Constants.h>
#include <Logger.h>

#include <algorithm>
#include <cmath>

namespace Nawia::Entity {

	SwordSlashEffect::SwordSlashEffect(const float x, const float y, const float angle, const std::shared_ptr<Texture2D>& tex, const AbilityStats& stats)
		: AbilityEffect(x, y, tex, stats), _angle(angle) {}

	void SwordSlashEffect::update(const float dt)
	{
		AbilityEffect::update(dt);
	}

	void SwordSlashEffect::render(const float camera_x, const float camera_y)
	{
		if (!_texture) {
			Core::Logger::errorLog("SwordSlashEffect - Could not load texture.");
			return;
		}

		Core::Point2D screen_pos = getScreenPos(_pos->getX(), _pos->getY(), camera_x, camera_y);

		const float source_texture_width = static_cast<float>(_texture->width);
		const float source_texture_height = static_cast<float>(_texture->height);
		constexpr float dest_texture_width = static_cast<float>(Core::ENTITY_TEXTURE_WIDTH);
		constexpr float dest_texture_height = static_cast<float>(Core::ENTITY_TEXTURE_HEIGHT);

		const Rectangle source = { 0.0f, 0.0f, source_texture_width, source_texture_height };
		const Rectangle dest = { screen_pos.getX(), screen_pos.getY(), dest_texture_width, dest_texture_height };
		constexpr Vector2 origin = { dest_texture_width, dest_texture_height};

		DrawTexturePro(*_texture, source, dest, origin, _angle, WHITE);
	}

	bool SwordSlashEffect::checkCollision(const std::shared_ptr<Entity>& target) const
	{
		const auto enemy = std::dynamic_pointer_cast<EnemyInterface>(target);

		if (!enemy || enemy->isDead() || hasHit(enemy))
			return false;

		// checks if the target is within the radius of the slash attack before calculating the angle
		const float dx = enemy->getX() - getX();
		const float dy = enemy->getY() - getY();
		const float distance_squared = dx * dx + dy * dy;

		const float range = _stats.hitbox_radius;
		if (distance_squared > range * range)
			return false;

		const float angle_to_enemy = static_cast<float>(std::atan2(dy, dx) * 180.0f / Core::pi);
		const float slash_angle = getAngle();
		const float angle_diff = std::remainder(angle_to_enemy - slash_angle, 360.0f);
		return std::abs(angle_diff) < 45.0f;
	}

	void SwordSlashEffect::onCollision(const std::shared_ptr<Entity>& target)
	{
		if (const auto enemy = std::dynamic_pointer_cast<EnemyInterface>(target))
		{
			enemy->takeDamage(getDamage());
			addHit(enemy);
			Core::Logger::debugLog("Sword Slash Hit Enemy!");
		}
	}

} // namespace Nawia::Entity