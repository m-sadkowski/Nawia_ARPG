#pragma once

#include <EnemyInterface.h>

#include <memory>

namespace Nawia::Entity {

	/**
	 * @class Devil
	 * @brief Agresywny demon z atakiem doskoku.
	 *
	 * Devil blokuje pozycję celu przed doskokiem, dzięki czemu atak jest
	 * czytelny i możliwy do uniknięcia przez gracza.
	 */
	class Devil : public EnemyInterface {
	public:
		/** @brief Tworzy demona w podanym punkcie mapy. */
		Devil(float x, float y, Core::Map* map);

		/** @brief Aktualizuje maszynę stanów demona. */
		void update(float dt) override;
		[[nodiscard]] Vector3 getWorldPos3D() const override;

	private:
		Devil();
		friend class DevilBuilder;

		enum class State {
			Idle,
			Chasing,
			PreparingDash,
			Dashing,
			JumpingShockwave,
			Recovering,
			Attacking
		};

		State _state = State::Idle;

		// Prędkości animacji.
		static constexpr float DEVIL_DASH_ANIMATION_SPEED = 0.85f;
		static constexpr float DEVIL_WALK_ANIMATION_SPEED = 1.0f;
		static constexpr float DEVIL_DEAD_ANIMATION_SPEED = 2.0f;
		static constexpr float DEVIL_ATTACK_ANIMATION_SPEED = 1.28f;
		static constexpr float DEVIL_SHOCKWAVE_IDLE_ANIMATION_SPEED = 2.0f;

		// Statystyki walki.
		static constexpr float VISION_RANGE = 20.0f;
		static constexpr float ATTACK_RANGE = 2.0f;
		static constexpr float SPEED = 0.5f;
		static constexpr int ATTACK_DAMAGE = 50;
		static constexpr float ATTACK_COOLDOWN = 1.0f;

		// Statystyki doskoku.
		static constexpr float DASH_TRIGGER_RANGE = 9.0f;    // Dystans, od którego doskok może się aktywować.
		static constexpr float DASH_SPEED = 8.0f;            // Prędkość doskoku.
		static constexpr float DASH_COOLDOWN = 3.0f;         // Czas między doskokami.
		static constexpr float DASH_PREPARE_TIME = 0.5f;     // Czas telegrafowania przed startem.
		static constexpr float DASH_ARRIVE_THRESHOLD = 0.3f; // Dystans uznawany za dotarcie do punktu.
		static constexpr int DASH_DAMAGE = 35;               // Obrażenia przy trafieniu doskokiem.
		static constexpr float DASH_HIT_RANGE = 3.5f;        // Zasięg sprawdzania trafienia podczas doskoku.
		static constexpr float DASH_STUN_DURATION = 0.7f;    // Czas odzyskiwania kontroli po doskoku.
		static constexpr float DASH_IMPACT_HAZARD_RADIUS = 2.35f;
		static constexpr float SCORCHED_GROUND_RADIUS = 1.65f;
		static constexpr float SCORCHED_GROUND_DURATION = 3.2f;
		static constexpr float BOSS_DASH_RECOVERY_DURATION = 0.32f;
		static constexpr float BOSS_SHOCKWAVE_RECOVERY_DURATION = 0.22f;
		static constexpr float SHOCKWAVE_TRIGGER_RANGE = 7.5f;
		static constexpr float SHOCKWAVE_WINDUP_TIME = 0.72f;
		static constexpr float SHOCKWAVE_MIN_RADIUS = 4.0f;
		static constexpr float SHOCKWAVE_MAX_RADIUS = 7.0f;
		static constexpr float SHOCKWAVE_SPEED = 4.0f;
		static constexpr float SHOCKWAVE_WIDTH = 0.95f;
		static constexpr float SHOCKWAVE_JUMP_HEIGHT = 1.15f;
		static constexpr int SHOCKWAVE_DAMAGE = 14;
		static constexpr int SHOCKWAVE_CHANCE_PERCENT = 38;

		float _attack_cooldown_timer = 0.0f;
		float _dash_cooldown_timer = 0.0f;
		float _dash_prepare_timer = 0.0f;
		Vector2 _dash_target_pos = {0.0f, 0.0f}; // Zablokowana pozycja celu dla doskoku.
		bool _dash_hit_target = false;           // Czy ten doskok już trafił cel.
		float _stun_timer = 0.0f;                // Licznik odzyskiwania kontroli po doskoku.
		float _shockwave_timer = 0.0f;
		float _shockwave_radius = SHOCKWAVE_MIN_RADIUS;
		float _visual_jump_height = 0.0f;
		bool _shockwave_impact_played = false;

		// Obsługa stanów.
		void configureModel();
		void handleIdleState(float dt);
		void handleChasingState(float dt);
		void handlePreparingDashState(float dt);
		void handleDashingState(float dt);
		void handleJumpingShockwaveState(float dt);
		void handleRecoveringState(float dt);
		void handleAttackingState(float dt);
		void startAttack();
		void startDash(const Vector2& target_pos);
		void startShockwaveJump();
		void finishRecovery();
		[[nodiscard]] float dashRecoveryDuration() const;
		[[nodiscard]] bool isBossVariant() const;
		void spawnDashImpactHazard();
		void spawnScorchedGroundHazard();
		void spawnShockwaveHazard();
		void onDeathStarted() override;
	};

	/**
	 * @class DevilBuilder
	 * @brief Builder konfigurujący instancję `Devil`.
	 */
	class DevilBuilder : public EnemyBuilder<DevilBuilder> {
	public:
		/** @brief Tworzy roboczą instancję demona. */
		DevilBuilder() {
			_devil_ptr = std::unique_ptr<Devil>(new Devil());
			this->_entity = _devil_ptr.get();
		}

		/** @brief Oddaje gotową instancję demona. */
		std::unique_ptr<Devil> build() {
			return std::move(_devil_ptr);
		}

	private:
		std::unique_ptr<Devil> _devil_ptr;
	};

} // namespace Nawia::Entity
