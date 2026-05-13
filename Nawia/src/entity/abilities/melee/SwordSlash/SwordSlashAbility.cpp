#include "SwordSlashAbility.h"

#include <Entity.h>
#include <Player.h>
#include <SoundIds.h>
#include <SwordSlashEffect.h>

namespace Nawia::Entity {

	namespace {
		constexpr float EFFECT_SPAWN_ANIMATION_RATIO = 0.6f;
		constexpr float ENTITY_ANIMATION_BASE_FPS = 60.0f;
		constexpr const char* PLAYER_SWORD_ATTACK_ANIM = "Sword_Regular_A";
	}

	SwordSlashAbility::SwordSlashAbility(const std::shared_ptr<Texture2D>& slash_tex,
										 const std::shared_ptr<Texture2D>& icon_tex)
		: Ability("Sword Slash", Entity::getAbilityStatsFromJson("Sword Slash"), AbilityTargetType::POINT, icon_tex),
		  _slash_tex(slash_tex) {}

	AbilitySpawn SwordSlashAbility::cast(const float target_x, const float target_y) {
		if (!beginCast())
			return nullptr;

		// Od razu obracamy źródło użycia w stronę celu.
		_caster->rotateTowardsCenter(target_x, target_y);

		if (const auto player = dynamic_cast<Player*>(_caster)) {
			// Gracz skaluje prędkość animacji ataku statystyką attack_speed.
			_caster->setAnimationSpeed(Player::ATTACK_ANIM_BASE_SPEED * player->getStats().attack_speed);
		}

		_caster->playAnimation(PLAYER_SWORD_ATTACK_ANIM, false, true, 0, true);
		_caster->playSoundEffect(Audio::SoundId::SwordSlash, 0.85f);

		// Zapisujemy stan opóźnionego utworzenia efektu.
		_is_active = true;
		_has_spawned = false;
		_active_time = 0.0f;
		_spawn_delay = calculateSpawnDelay();

		return nullptr;
	}

	void SwordSlashAbility::update(const float dt) {
		Ability::update(dt);

		if (!_is_active || !_caster)
			return;

		_active_time += dt;

		// Utworzenie następuje po opóźnieniu albo awaryjnie po odblokowaniu animacji.
		const bool should_spawn = !_has_spawned && (_active_time >= _spawn_delay || !_caster->isAnimationLocked());
		if (should_spawn)
			spawnSlashEffect();

		// Kończymy stan ability, gdy animacja przestaje blokować ruch.
		if (!_caster->isAnimationLocked())
			_is_active = false;
	}

	float SwordSlashAbility::calculateSpawnDelay() const {
		if (!_caster)
			return 0.0f;

		const int frames = _caster->getAnimationFrameCount(PLAYER_SWORD_ATTACK_ANIM);
		const float duration = (frames > 0) ? (static_cast<float>(frames) / ENTITY_ANIMATION_BASE_FPS) : 1.0f;

		// Trafienie pojawia się w okolicy punktu impaktu animacji.
		return duration * EFFECT_SPAWN_ANIMATION_RATIO;
	}

	void SwordSlashAbility::spawnSlashEffect() {
		if (!_caster)
			return;

		_has_spawned = true;

		const Vector2 caster_center = _caster->getCenter();
		const float angle = _caster->getRotation();

		const auto slash = std::make_shared<SwordSlashEffect>(
			caster_center.x,
			caster_center.y,
			-angle,
			_slash_tex,
			_stats,
			_caster);
		slash->setAltitude(_caster->getAltitude());

		_caster->addPendingSpawn(slash);
	}

} // namespace Nawia::Entity
