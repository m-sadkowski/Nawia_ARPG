#include "SwordSlashEffect.h"
#include "EnemyInterface.h"
#include "Collider.h"
#include "../../../actors/player/Player.h"

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

	void SwordSlashEffect::render(const Camera3D& camera)
	{
		// For now, just render debug collider in 3D (the 2D slash texture is legacy)
		// TODO: Add 3D slash visual effect
		if (DebugColliders && _collider) {
			_collider->render(camera);
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
