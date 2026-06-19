#include "UnarmedMeleeAbility.h"

#include <Entity.h>
#include <EnemyInterface.h>
#include <Logger.h>
#include <Player.h>

#include <algorithm>
#include <string>
#include <utility>

namespace Nawia::Entity {

	namespace {
		constexpr float ENTITY_ANIMATION_BASE_FPS = 60.0f;
	}

	UnarmedMeleeAbility::UnarmedMeleeAbility(
		std::string ability_name,
		const std::string& stats_key,
		std::string animation_name,
		const AbilityTargetType target_type,
		const bool direct_target_hit,
		const UnarmedMeleeEffect::Shape effect_shape,
		const float spawn_ratio,
		const float hitbox_width,
		const float knockback_distance,
		const bool ping_pong_animation,
		const std::shared_ptr<Texture2D>& icon_tex)
		: Ability(std::move(ability_name), Entity::getAbilityStatsFromJson(stats_key), target_type, icon_tex),
		  _animation_name(std::move(animation_name)),
		  _direct_target_hit(direct_target_hit),
		  _effect_shape(effect_shape),
		  _spawn_ratio(spawn_ratio),
		  _hitbox_width(hitbox_width),
		  _knockback_distance(knockback_distance),
		  _ping_pong_animation(ping_pong_animation) {}

	AbilitySpawn UnarmedMeleeAbility::cast(const float target_x, const float target_y) {
		if (!beginCast())
			return nullptr;

		_caster->rotateTowardsCenter(target_x, target_y);

		if (const auto player = dynamic_cast<Player*>(_caster))
			_caster->setAnimationSpeed(Player::ATTACK_ANIM_BASE_SPEED * player->getStats().attack_speed);
		else
			_caster->setAnimationSpeed(1.5f);

		if (_ping_pong_animation)
			_caster->playAnimationPingPong(_animation_name, true, 0, true);
		else
			_caster->playAnimation(_animation_name, false, true, 0, true);

		if (_direct_target_hit) {
			// Najprostsze ciosy moga ominac efekt kolizyjny i uderzyc aktualny target w zasiegu.
			const auto target = _caster->getTarget();
			const auto enemy = std::dynamic_pointer_cast<EnemyInterface>(target);
			if (enemy && _caster->getDistanceToTarget() <= _stats.cast_range) {
				int final_damage = _stats.damage;
				if (const auto player = dynamic_cast<Player*>(_caster))
					final_damage += std::max(0, player->getStats().damage / 2);

				enemy->rememberDamageSource(_caster);
				enemy->takeDamage(final_damage);
				Core::Logger::debugLog(
					getName() + " trafil " + enemy->getName() + " za " + std::to_string(final_damage) + " obrazen.");
			}
			return nullptr;
		}

		_is_active = true;
		_has_spawned = false;
		_active_time = 0.0f;
		_spawn_delay = calculateAnimationDuration() * _spawn_ratio;

		return nullptr;
	}

	void UnarmedMeleeAbility::update(const float dt) {
		Ability::update(dt);

		if (!_is_active || !_caster)
			return;

		_active_time += dt;
		// Hitbox pojawia sie w wybranym procencie animacji albo awaryjnie, gdy animacja odblokuje ruch szybciej.
		if (!_has_spawned && (_active_time >= _spawn_delay || !_caster->isAnimationLocked()))
			spawnEffect();

		if (!_caster->isAnimationLocked())
			_is_active = false;
	}

	void UnarmedMeleeAbility::cancel() {
		_is_active = false;
		_has_spawned = false;
		_active_time = 0.0f;
		_spawn_delay = 0.0f;
	}

	float UnarmedMeleeAbility::calculateAnimationDuration() const {
		if (!_caster)
			return 0.35f;

		const int frames = _caster->getAnimationFrameCount(_animation_name);
		const float animation_speed = std::max(0.01f, _caster->getAnimationSpeed());
		return (frames > 0)
			? (static_cast<float>(frames) * Entity::ANIMATION_DURATION_SCALE / (ENTITY_ANIMATION_BASE_FPS * animation_speed))
			: 0.35f;
	}

	void UnarmedMeleeAbility::spawnEffect() {
		if (!_caster)
			return;

		_has_spawned = true;

		// Efekt jest osobna encja, bo system kolizji ability przetwarza ja poza logika samego castera.
		const Vector2 caster_center = _caster->getCenter();
		const float angle = _caster->getRotation();
		auto effect = std::make_shared<UnarmedMeleeEffect>(
			caster_center.x,
			caster_center.y,
			-angle,
			_stats,
			_caster,
			_effect_shape,
			_hitbox_width,
			_knockback_distance);
		effect->setAltitude(_caster->getAltitude());
		_caster->addPendingSpawn(effect);
	}

} // namespace Nawia::Entity
