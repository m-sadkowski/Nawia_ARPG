#pragma once

#include <EnemyInterface.h>

#include <memory>

namespace Nawia::Entity {

	/**
	 * @class WalkingDead
	 * @brief Powolny nieumarły z reakcją na trafienie i zmienną prędkością.
	 *
	 * Walking Dead przyspiesza po zbliżeniu się do celu, a otrzymane obrażenia
	 * przerywają jego bieżący stan krótką animacją trafienia.
	 */
	class WalkingDead : public EnemyInterface {
	public:
		/** @brief Tworzy nieumarłego w podanym punkcie mapy. */
		WalkingDead(float x, float y, Core::Map* map);

		/** @brief Aktualizuje maszynę stanów i ruch nieumarłego. */
		void update(float dt) override;

		/** @brief Zadaje obrażenia i uruchamia reakcję na trafienie. */
		void takeDamage(int dmg) override;

	private:
		WalkingDead();
		friend class WalkingDeadBuilder;

		enum class State {
			Idle,
			Chasing,
			Attacking,
			Screaming,
			GettingHit
		};

		State _state = State::Idle;
		State _state_before_hit = State::Idle; // Stan przywracany po animacji trafienia.

		// Statystyki walki.
		static constexpr float VISION_RANGE = 10.0f;
		static constexpr float CLOSE_RANGE = VISION_RANGE / 2.0f; // Dystans, od którego wróg zaczyna biec.
		static constexpr float ATTACK_RANGE = 1.0f;
		static constexpr float SPEED = 1.0f;
		static constexpr float RUN_SPEED = 3.0f; // Prędkość biegu blisko gracza.
		static constexpr int ATTACK_DAMAGE = 25;
		static constexpr float ATTACK_COOLDOWN = 1.2f;
		static constexpr float ATTACK_ANIMATION_SPEED = 2.25f;
		static constexpr float HIT_REACTION_ANIMATION_SPEED = 1.4f;
		static constexpr float DEFAULT_ANIMATION_SPEED = 1.0f;
		static constexpr float ATTACK_DAMAGE_FRAME_RATIO = 0.38f;
		static constexpr int HIT_INTERRUPT_CHANCE = 45;

		float _attack_cooldown_timer = 0.0f;
		bool _attack_damage_applied = false;
		bool _is_running = false; // Czy aktualnie odtwarzamy wariant biegu.
		float _ambient_sound_timer = 0.0f;

		// Ruch i wyznaczanie ścieżki.
		static constexpr float DIRECT_MOVE_DISTANCE = 2.0f;

		// Obsługa stanów.
		void handleIdleState(float dt);
		void handleChasingState(float dt);
		void handleAttackingState(float dt);
		void handleScreamingState(float dt);
		void handleGettingHitState(float dt);
		void updateAmbientSound(float dt);
		void onDeathStarted() override;
	};

	/**
	 * @class WalkingDeadBuilder
	 * @brief Builder konfigurujący instancję `WalkingDead`.
	 */
	class WalkingDeadBuilder : public EnemyBuilder<WalkingDeadBuilder> {
	public:
		/** @brief Tworzy roboczą instancję nieumarłego. */
		WalkingDeadBuilder() {
			_walkingdead_ptr = std::unique_ptr<WalkingDead>(new WalkingDead());
			this->_entity = _walkingdead_ptr.get();
		}

		/** @brief Oddaje gotową instancję nieumarłego. */
		std::unique_ptr<WalkingDead> build() {
			return std::move(_walkingdead_ptr);
		}

	private:
		std::unique_ptr<WalkingDead> _walkingdead_ptr;
	};

} // namespace Nawia::Entity
