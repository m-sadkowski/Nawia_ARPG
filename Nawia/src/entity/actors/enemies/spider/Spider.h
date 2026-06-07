#pragma once

#include <EnemyInterface.h>

#include <memory>
#include <string>
#include <vector>

namespace Nawia::Entity {

	class Spider : public EnemyInterface {
	public:
		void update(float dt) override;
		void takeDamage(int dmg) override;

	private:
		Spider();
		friend class SpiderBuilder;

		enum class State {
			Idle,
			Chasing,
			MeleeAttacking,
			WebAttacking,
			GettingHit
		};

		void configureModel();
		void handleIdleState(float dt);
		void handleChasingState(float dt);
		void handleMeleeAttackingState(float dt);
		void handleWebAttackingState(float dt);
		void handleGettingHitState(float dt);
		void startMeleeAttack();
		void startWebAttack();
		void fireWeb();
		void stopMoving();
		void playIdle();
		void playWalk(float speed_multiplier = 1.0f);
		bool moveTowardPositionWithNav(Vector2 target_pos, float dt, float repath_interval = 0.3f);
		bool canReachPositionWithNav(Vector2 target_pos) const;
		void clearNavigationPath();
		void onDeathStarted() override;

		State _state = State::Idle;
		State _state_before_hit = State::Idle;
		float _melee_cooldown_timer = 0.0f;
		float _web_cooldown_timer = 1.0f;
		bool _melee_damage_applied = false;
		bool _web_fired = false;
		std::vector<Vector2> _current_nav_path;
		Vector2 _current_nav_target = {0.0f, 0.0f};
		bool _has_current_nav_target = false;

		static constexpr float MODEL_SCALE = 0.7f;
		static constexpr float VISION_RANGE = 16.0f;
		static constexpr float MELEE_RANGE = 1.45f;
		static constexpr float WEB_RANGE = 10.0f;
		static constexpr float WEB_MIN_RANGE = 2.4f;
		static constexpr float MOVE_SPEED = 3.0f;
		static constexpr float WEB_CHASE_SPEED_MULTIPLIER = 1.25f;
		static constexpr float MELEE_COOLDOWN = 1.35f;
		static constexpr float WEB_COOLDOWN = 4.2f;
		static constexpr float MELEE_ANIMATION_SPEED = 1.15f;
		static constexpr float WEB_ANIMATION_SPEED = 1.0f;
		static constexpr float MELEE_DAMAGE_FRAME_RATIO = 0.45f;
		static constexpr float WEB_FIRE_FRAME_RATIO = 0.45f;
		static constexpr int MELEE_DAMAGE = 25;
		static constexpr float POISON_DURATION = 5.0f;
		static constexpr int POISON_TICK_DAMAGE = 5;
	};

	class SpiderBuilder : public EnemyBuilder<SpiderBuilder> {
	public:
		SpiderBuilder() {
			_spider_ptr = std::unique_ptr<Spider>(new Spider());
			this->_entity = _spider_ptr.get();
		}

		std::unique_ptr<Spider> build() {
			return std::move(_spider_ptr);
		}

	private:
		std::unique_ptr<Spider> _spider_ptr;
	};

} // namespace Nawia::Entity
