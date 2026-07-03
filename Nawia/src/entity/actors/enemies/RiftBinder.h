#pragma once

#include <EnemyInterface.h>

#include <memory>
#include <string>
#include <vector>

namespace Nawia::Entity {

	class RiftTotem;

	/**
	 * @brief Stage-based raid boss using the dragon model as the Siewca Chaosu encounter.
	 *
	 * The boss locks itself behind totem stages. Players and future agents must
	 * destroy 3, 4, 5, then 7 totems to make the boss vulnerable again while it
	 * pressures the arena with stone projectiles, random teleports, and fire rain.
	 */
	class RiftBinder : public EnemyInterface {
	public:
		RiftBinder(float x, float y, Core::Map* map);

		void update(float dt) override;
		void takeDamage(int dmg) override;
		[[nodiscard]] Vector3 getWorldPos3D() const override;
		[[nodiscard]] bool isVisibleInCamera(const Camera3D& camera, float screen_margin = 96.0f) const override;
		void setHelperModelOverride(std::string model_path, float scale);
		void setStoneProjectileModel(std::string model_path, float scale);

	private:
		RiftBinder();
		friend class RiftBinderBuilder;

		enum class State {
			Idle,
			Repositioning,
			Casting,
			Recovering,
			MeleeAttacking,
			HitReacting
		};

		enum class Spell {
			None,
			StoneVolley,
			FireRain,
			DragonBlink
		};

		void configureModel();
		void handleIdleState(float dt);
		void handleRepositioningState(float dt);
		void handleCastingState(float dt);
		void handleRecoveringState(float dt);
		void handleMeleeAttackingState(float dt);
		void handleHitReactingState(float dt);
		void updateTotemStage();

		void tryStartAction();
		void startStoneVolleyCast();
		void startFireRainCast();
		void startDragonBlinkCast();
		void startSpellCast(Spell spell, const char* cast_name, const char* animation_name, float cast_time);
		void finishSpellCast();
		void startMeleeAttack();
		void startHitReact();

		void startTotemStage(int stage_index);
		void spawnStageTotems(int stage_index);
		void spawnStoneVolley();
		void spawnBlinkFlare(Vector2 position);
		void spawnFireRain(float warning_seconds);
		void performRandomTeleport();
		void applyMeleeDamage();

		void moveAwayFromTarget(float dt);
		void chaseToPreferredRange(float dt);
		void stopMoving();
		void playIdle();
		void playWalk();
		void onDeathStarted() override;

		[[nodiscard]] Vector2 findWalkableNearby(Vector2 preferred, Vector2 fallback) const;
		[[nodiscard]] bool isReachableWalkable(Vector2 from, Vector2 position) const;
		[[nodiscard]] Vector2 findTeleportDestination() const;
		[[nodiscard]] Vector2 targetCenterOrSelf() const;
		[[nodiscard]] int livingTotemCount();
		[[nodiscard]] std::vector<std::shared_ptr<RiftTotem>> liveTotems();
		[[nodiscard]] float actionCooldownDuration() const;
		[[nodiscard]] bool canCastFireRain();
		[[nodiscard]] float currentFireRainCooldown();

		State _state = State::Idle;
		Spell _casting_spell = Spell::None;
		bool _melee_damage_applied = false;
		bool _shield_active = false;
		int _active_stage = -1;
		int _next_stage_to_start = 0;

		float _action_cooldown_timer = 0.55f;
		float _stone_cooldown_timer = 0.35f;
		float _fire_rain_cooldown_timer = 2.2f;
		float _blink_cooldown_timer = 6.0f;
		float _melee_cooldown_timer = 0.0f;
		float _cast_timer = 0.0f;
		float _recover_timer = 0.0f;
		float _hit_react_timer = 0.0f;

		std::vector<std::weak_ptr<RiftTotem>> _totems;
		std::string _helper_model_path = "assets/models/actors/walking_dead/walking_dead_2.glb";
		float _helper_model_scale = 1.5f;
		std::string _stone_projectile_model_path = "assets/models/fireball.glb";
		float _stone_projectile_model_scale = 0.3f;

		static constexpr float DRAGON_TARGET_HEIGHT = 1.25f;
		static constexpr float VISION_RANGE = 24.0f;
		static constexpr float LEASH_RANGE = 36.0f;
		static constexpr float CAST_RANGE = 15.5f;
		static constexpr float MIN_DISTANCE = 4.2f;
		static constexpr float PREFERRED_DISTANCE = 8.0f;
		static constexpr float MOVE_SPEED = 2.45f;
		static constexpr float MELEE_RANGE = 2.1f;
		static constexpr float DRAGON_VISUAL_HEIGHT_OFFSET = 0.45f;

		static constexpr float STONE_VOLLEY_CAST_TIME = 0.48f;
		static constexpr float FIRE_RAIN_CAST_TIME = 1.45f;
		static constexpr float DRAGON_BLINK_CAST_TIME = 0.28f;
		static constexpr float RECOVERY_TIME = 0.22f;
		static constexpr float HIT_REACT_TIME = 0.34f;
		static constexpr float ACTION_COOLDOWN = 1.35f;
		static constexpr float SHIELDED_ACTION_COOLDOWN = 1.15f;
		static constexpr float STONE_COOLDOWN = 2.80f;
		static constexpr float BLINK_COOLDOWN = 16.0f;

		static constexpr float TOTEM_RING_RADIUS = 7.15f;
		static constexpr float TOTEM_RING_RADIUS_JITTER = 0.95f;
		static constexpr float TOTEM_TANGENTIAL_JITTER = 0.65f;
		static constexpr float TOTEM_MIN_SEPARATION = 2.15f;
		static constexpr int TOTEM_POSITION_ATTEMPTS = 8;
		static constexpr int STONE_PROJECTILE_COUNT = 3;
		static constexpr float STONE_PROJECTILE_SPEED = 8.6f;
		static constexpr float STONE_PROJECTILE_HIT_RADIUS = 0.42f;
		static constexpr float STONE_PROJECTILE_DURATION = 3.0f;
		static constexpr float STONE_PROJECTILE_SPREAD = 0.85f;
		static constexpr float STONE_PROJECTILE_SPAWN_FORWARD = 0.78f;
		static constexpr float STONE_PROJECTILE_SPAWN_SIDE = 0.32f;
		static constexpr float BLINK_FLARE_RADIUS = 0.95f;
		static constexpr float BLINK_MIN_RADIUS = 4.4f;
		static constexpr float BLINK_MAX_RADIUS = 7.0f;
		static constexpr int STONE_DAMAGE = 13;
		static constexpr float FIRE_RAIN_RADIUS = 4.1f;
		static constexpr float FIRE_RAIN_ACTIVE_SECONDS = 3.6f;
		static constexpr int FIRE_RAIN_DAMAGE = 9;

		static constexpr int MELEE_DAMAGE = 22;
		static constexpr float MELEE_COOLDOWN = 1.25f;
		static constexpr float MELEE_ANIMATION_SPEED = 1.35f;
		static constexpr float HIT_REACT_ANIMATION_SPEED = 1.25f;
		static constexpr float MELEE_DAMAGE_FRAME_RATIO = 0.42f;
		static constexpr float MELEE_ROOT_SECONDS = 0.18f;
	};

	class RiftBinderBuilder : public EnemyBuilder<RiftBinderBuilder> {
	public:
		RiftBinderBuilder() {
			_rift_binder_ptr = std::unique_ptr<RiftBinder>(new RiftBinder());
			this->_entity = _rift_binder_ptr.get();
		}

		std::unique_ptr<RiftBinder> build() {
			return std::move(_rift_binder_ptr);
		}

	private:
		std::unique_ptr<RiftBinder> _rift_binder_ptr;
	};

} // namespace Nawia::Entity
