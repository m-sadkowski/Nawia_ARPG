#include "SwordSlashAbility.h"
#include <SwordSlashEffect.h>
#include <Entity.h>
#include <Player.h>

#include <MathUtils.h>

namespace Nawia::Entity {

	SwordSlashAbility::SwordSlashAbility(const std::shared_ptr<Texture2D>& slash_tex, const std::shared_ptr<Texture2D>& icon_tex)
	    : Ability("Sword Slash", Entity::getAbilityStatsFromJson("Sword Slash"), AbilityTargetType::POINT, icon_tex), _slash_tex(slash_tex) {}

	std::unique_ptr<Entity> SwordSlashAbility::cast(const float target_x, const float target_y) 
	{
		if (!isReady()) return nullptr;

		// Od razu obracamy źródło użycia w stronę celu.
		_caster->rotateTowardsCenter(target_x, target_y);

		// Gracz skaluje prędkość animacji ataku statystyką attack_speed.
		Player* player = dynamic_cast<Player*>(_caster);

		if (player != nullptr) {
			_caster->rotateTowardsCenter(target_x, target_y);
			_caster->setAnimationSpeed(player->ATTACK_ANIM_BASE_SPEED*player->getStats().attack_speed);
			_caster->playAnimation("attack", false, true); 
		}
		else {
			
			_caster->rotateTowardsCenter(target_x, target_y);
			_caster->playAnimation("attack", false, true);
		}


		startCooldown();
		
		_caster->playAnimation("attack", false, true);

		// Zapisujemy cel i aktywujemy opóźnione utworzenie efektu.
		_is_active = true;
		_has_spawned = false;
		_active_time = 0.0f;
		_target_x = target_x;
		_target_y = target_y;

		// Opóźnienie utworzenia efektu liczymy na podstawie długości animacji.
		// Entity aktualizuje animacje z bazowym tempem 60 FPS.
		const int frames = _caster->getAnimationFrameCount("attack");
		const float duration = (frames > 0) ? (frames / 60.0f) : 1.0f; 
		
		// Trafienie pojawia się w okolicy punktu impaktu animacji.
		_spawn_delay = duration * 0.6f;

		// Efekt pojawi się później w aktualizacji, więc teraz nic nie zwracamy.
		return nullptr;
	}

	void SwordSlashAbility::update(const float dt)
	{
		Ability::update(dt);

		if (_is_active)
		{
			_active_time += dt;

		// Utworzenie następuje po opóźnieniu albo awaryjnie po odblokowaniu animacji.
			bool should_spawn = !_has_spawned && (_active_time >= _spawn_delay || !_caster->isAnimationLocked());

			if (should_spawn)
			{
				_has_spawned = true;

				const Vector2 caster_center = _caster->getCenter();

			// Jeśli źródło użycia obróciło się w trakcie rzutu, cięcie idzie w aktualnym kierunku patrzenia.
				const float angle = _caster->getRotation();
				const float spawn_x = caster_center.x;
				const float spawn_y = caster_center.y;

				const auto slash = std::make_shared<SwordSlashEffect>(spawn_x, spawn_y, -angle, _slash_tex, _stats, _caster);
				_caster->addPendingSpawn(slash);
			}

			// Kończymy stan ability, gdy animacja przestaje blokować ruch.
			if (!_caster->isAnimationLocked())
			{
				_is_active = false;
			}
		}
	}

} // namespace Nawia::Entity
