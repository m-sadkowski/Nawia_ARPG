#pragma once

#include <EnemyInterface.h>

#include <memory>

namespace Nawia::Entity {

	class Player;

	/**
	 * @brief Czarownica trzymajaca dystans, strzelajaca pociskami i wzywajaca pomagiery.
	 */
	class Witch : public EnemyInterface {
	public:
		Witch(float x, float y, Core::Map* map);

		void update(float dt) override;
		void takeDamage(int dmg) override;

	private:
		Witch();
		friend class WitchBuilder;

		enum class State {
			Idle,
			Repositioning,
			CastingBolt,
			GettingHit,
			Retaliating,
			Summoning
		};

		void configureModel();
		void handleIdleState(float dt);
		void handleRepositioningState(float dt);
		void handleCastingBoltState(float dt);
		void handleGettingHitState(float dt);
		void handleRetaliatingState(float dt);
		void handleSummoningState(float dt);
		void startBoltCast();
		void fireBolt();
		void startRetaliation();
		void startImmediateSummonRetaliation();
		void applyRetaliation();
		void pushRetaliationTarget(Player& target) const;
		void summonHelper();
		void moveAwayFromTarget(float dt);
		void chaseToCastRange(float dt);
		void stopMoving();
		void updateDeathFreeze(float dt);
		void playIdle();
		void playRun();
		void onDeathStarted() override;

		State _state = State::Idle;
		float _cast_cooldown_timer = 0.5f;
		bool _cast_projectile_spawned = false;
		bool _retaliation_applied = false;
		bool _reposition_after_retaliation = false;

		static constexpr float MODEL_SCALE = 1.55f;
		static constexpr float VISION_RANGE = 20.0f;
		static constexpr float CAST_RANGE = 11.0f;
		static constexpr float MIN_DISTANCE = 6.0f;
		static constexpr float PREFERRED_DISTANCE = 8.5f;
		static constexpr float MOVE_SPEED = 3.4f;
		static constexpr float CAST_COOLDOWN = 2.4f;
		static constexpr float CAST_FRAME_RATIO = 0.34f;
		static constexpr float BOLT_ANIMATION_SPEED = 1.45f;
		static constexpr float RETALIATION_ANIMATION_SPEED = 1.75f;
		static constexpr float SUMMON_ANIMATION_SPEED = 1.6f;
		static constexpr float RETALIATION_PUSH_DISTANCE = 1.15f;
		static constexpr int BOLT_DAMAGE = 18;
		static constexpr int RETALIATION_DAMAGE = 8;
	};

	class WitchBuilder : public EnemyBuilder<WitchBuilder> {
	public:
		WitchBuilder() {
			_witch_ptr = std::unique_ptr<Witch>(new Witch());
			this->_entity = _witch_ptr.get();
		}

		std::unique_ptr<Witch> build() {
			return std::move(_witch_ptr);
		}

	private:
		std::unique_ptr<Witch> _witch_ptr;
	};

} // namespace Nawia::Entity
