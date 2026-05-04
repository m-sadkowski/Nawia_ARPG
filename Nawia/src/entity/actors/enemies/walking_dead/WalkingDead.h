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

		enum class State { Idle, Chasing, Attacking, Screaming, GettingHit };
		State _state = State::Idle;
		State _state_before_hit = State::Idle;  // State to return to after get_hit animation

		// Combat stats
		static constexpr float VISION_RANGE = 10.0f;
		static constexpr float CLOSE_RANGE = VISION_RANGE / 2;    // Distance at which zombie starts running
		static constexpr float ATTACK_RANGE = 1.0f;
		static constexpr float SPEED = 1.0f;
		static constexpr float RUN_SPEED = 3.0f;      // Fast speed when close to player
		static constexpr int ATTACK_DAMAGE = 25;
		static constexpr float ATTACK_COOLDOWN = 2.0f;
		
		float _attack_cooldown_timer = 0.0f;
		bool _is_running = false;  // Track if currently running
		
		// Pathfinding
		static constexpr float DIRECT_MOVE_DISTANCE = 2.0f;

		// State handlers
		void handleIdleState(float dt);
		void handleChasingState(float dt);
		void handleAttackingState(float dt);
		void handleScreamingState(float dt);
		void handleGettingHitState(float dt);
	};

	class WalkingDeadBuilder : public EnemyBuilder<WalkingDeadBuilder> {
	public:
		WalkingDeadBuilder() {
			_walkingdead_ptr = std::unique_ptr<WalkingDead>(new WalkingDead());

			this->_entity = _walkingdead_ptr.get();
		}

		std::unique_ptr<WalkingDead> build() {
			return std::move(_walkingdead_ptr);
		}
	private:
		std::unique_ptr<WalkingDead> _walkingdead_ptr;
	};

} // namespace Nawia::Entity
