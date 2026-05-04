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

	private:
		Devil();
		friend class DevilBuilder;

		enum class State { Idle, Chasing, PreparingDash, Dashing, Recovering, Attacking };
		State _state = State::Idle;

		//Animation speed set
		static constexpr float DEVIL_DASH_ANIMATION_SPEED = 3.0f;
		static constexpr float DEVIL_WALK_ANIMATION_SPEED = 1.0f;
		static constexpr float DEVIL_DEAD_ANIMATION_SPEED = 2.0f;
		static constexpr float DEVIL_ATTACK_ANIMATION_SPEED = 1.0f;
		
		// Combat stats
		static constexpr float VISION_RANGE = 20.0f;
		static constexpr float ATTACK_RANGE = 2.f;
		static constexpr float SPEED = 0.5f;
		static constexpr int ATTACK_DAMAGE = 50;
		static constexpr float ATTACK_COOLDOWN = 1.5f;
		
		// Dash stats
		static constexpr float DASH_TRIGGER_RANGE = 9.0f;   // Distance at which dash can trigger
		static constexpr float DASH_SPEED = 8.0f;           // Dash speed
		static constexpr float DASH_COOLDOWN = 4.0f;        // Seconds between dashes
		static constexpr float DASH_PREPARE_TIME = 0.5f;    // Telegraph time before dash
		static constexpr float DASH_ARRIVE_THRESHOLD = 0.3f;// Distance to consider arrived
		static constexpr int DASH_DAMAGE = 35;              // Damage dealt when dash hits
		static constexpr float DASH_HIT_RANGE = 3.5f;       // Range to check collision during dash
		static constexpr float DASH_STUN_DURATION = 2.0f;   // Stun duration after dash ends
		
		float _attack_cooldown_timer = 0.0f;
		float _dash_cooldown_timer = 0.0f;
		float _dash_prepare_timer = 0.0f;
		Vector2 _dash_target_pos = {0, 0};  // Locked position for dash
		bool _dash_hit_target = false;      // Did we already hit during this dash?
		float _stun_timer = 0.0f;           // Recovery/stun timer after dash
	
		// State handlers
		void handleIdleState(float dt);
		void handleChasingState(float dt);
		void handlePreparingDashState(float dt);
		void handleDashingState(float dt);
		void handleRecoveringState(float dt);
		void handleAttackingState(float dt);
	};
	
	class DevilBuilder : public EnemyBuilder<DevilBuilder> {
	public:
		DevilBuilder() {
			_devil_ptr = std::unique_ptr<Devil>(new Devil());

			this->_entity = _devil_ptr.get();
		}

		std::unique_ptr<Devil> build() {
			return std::move(_devil_ptr);
		}
	private:
		std::unique_ptr<Devil> _devil_ptr;
	};

} // namespace Nawia::Entity
