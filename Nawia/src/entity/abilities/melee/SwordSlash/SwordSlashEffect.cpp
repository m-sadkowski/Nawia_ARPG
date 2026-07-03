#include "SwordSlashEffect.h"

#include <Collider.h>
#include <EnemyInterface.h>
#include <Logger.h>
#include <Player.h>

#include <string>

namespace Nawia::Entity {

	namespace {
		constexpr float DEFAULT_SLASH_RADIUS = 1.5f;
		constexpr float SLASH_ANGLE_DEGREES = 90.0f;
	}

	SwordSlashEffect::SwordSlashEffect(const float x,
									   const float y,
									   const float angle,
									   const std::shared_ptr<Texture2D>& tex,
									   const AbilityStats& stats,
									   Entity* caster)
		: AbilityEffect("Sword Slash", x, y, tex, stats),
		  _angle(angle),
		  _caster(caster) {
		setRotation(angle);
		setCollider(std::make_unique<ConeCollider>(
			this,
			stats.hitbox_radius > 0.0f ? stats.hitbox_radius : DEFAULT_SLASH_RADIUS,
			SLASH_ANGLE_DEGREES));
	}

	void SwordSlashEffect::update(const float dt) {
		AbilityEffect::update(dt);
	}

	void SwordSlashEffect::render(const Camera3D& camera) {
		// Na razie renderujemy tylko diagnostyczny kolider; tekstura 2D jest starym wariantem.
		// DO ZROBIENIA: dodać trójwymiarowy efekt wizualny cięcia.
		if (DebugColliders && _collider)
			_collider->render(camera);
	}

	bool SwordSlashEffect::checkCollision(const std::shared_ptr<Entity>& target) const {
		if (!canHitTarget(target))
			return false;

		const auto enemy = std::dynamic_pointer_cast<EnemyInterface>(target);
		if (!enemy)
			return false;

		return AbilityEffect::checkCollision(target);
	}

	void SwordSlashEffect::onCollision(const std::shared_ptr<Entity>& target) {
		const auto enemy = std::dynamic_pointer_cast<EnemyInterface>(target);
		if (!enemy)
			return;

		int final_damage = getDamage();
		if (const auto player = dynamic_cast<Player*>(_caster))
			final_damage += player->getStats().damage;

		enemy->rememberDamageSource(_caster, getName());
		enemy->takeDamage(final_damage);
		addHit(enemy);
		Core::Logger::debugLog(
			"Sword Slash trafil " + enemy->getName() + " za " + std::to_string(final_damage) + " obrazen.");
	}

} // namespace Nawia::Entity
