#include "SwordSlashEffect.h"
#include "EnemyInterface.h"
#include "Collider.h"
#include "../../../actors/player/Player.h"

#include <Constants.h>
#include <Logger.h>

#include <algorithm>
#include <cmath>

namespace Nawia::Entity {

	SwordSlashEffect::SwordSlashEffect(const float x, const float y, const float angle, const std::shared_ptr<Texture2D>& tex, const AbilityStats& stats, Entity* caster)
		: AbilityEffect("Sword Slash", x, y, tex, stats), _angle(angle), _caster(caster)
	{
		setRotation(angle);
		setCollider(std::make_unique<ConeCollider>(this, stats.hitbox_radius > 0 ? stats.hitbox_radius : 1.5f, 90.0f));
	}

	void SwordSlashEffect::update(const float dt)
	{
		AbilityEffect::update(dt);
	}

	void SwordSlashEffect::render(const float camera_x, const float camera_y)
	{
		// render base (texture) only if we hit something
		if (!_hit_entities.empty())
		{
			if (!_texture) {
				Core::Logger::errorLog("SwordSlashEffect - Could not load texture.");
			}
			else {
				Vector2 screen_pos = getScreenPos(_pos.x, _pos.y, camera_x, camera_y);

				const float source_texture_width = static_cast<float>(_texture->width);
				const float source_texture_height = static_cast<float>(_texture->height);
				constexpr float dest_texture_width = static_cast<float>(Core::ENTITY_TEXTURE_WIDTH);
				constexpr float dest_texture_height = static_cast<float>(Core::ENTITY_TEXTURE_HEIGHT);

				const Rectangle source = { 0.0f, 0.0f, source_texture_width, source_texture_height };
				const Rectangle dest = { screen_pos.x, screen_pos.y, dest_texture_width, dest_texture_height };
				constexpr Vector2 origin = { dest_texture_width, dest_texture_height };

				DrawTexturePro(*_texture, source, dest, origin, _angle, WHITE);
			}
		}

		if (DebugColliders && _collider) {
			_collider->render(camera_x, camera_y);
		}
	}

	bool SwordSlashEffect::checkCollision(const std::shared_ptr<Entity>& target) const
	{
		const auto enemy = std::dynamic_pointer_cast<EnemyInterface>(target);

		if (!enemy || enemy->isDead() || hasHit(enemy))
			return false;

		return AbilityEffect::checkCollision(target);
	}

	void SwordSlashEffect::onCollision(const std::shared_ptr<Entity>& target)
	{
		if (const auto enemy = std::dynamic_pointer_cast<EnemyInterface>(target))
		{
			int final_damage = getDamage();

			if (_caster) {
				if (const auto player = dynamic_cast<Player*>(_caster)) {
					final_damage += player->getStats().damage;
				}
			}

			enemy->takeDamage(final_damage);
			addHit(enemy);
			Core::Logger::debugLog("Sword Slash Hit " + enemy->getName() + " for " + std::to_string(final_damage) + " damage.");
		}
	}

} // namespace Nawia::Entity
