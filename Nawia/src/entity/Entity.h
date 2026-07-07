#pragma once

#include <EntityTypes.h>

#include <json.hpp>
#include <raylib.h>

#include <memory>
#include <string>
#include <vector>

namespace Nawia::Entity {
	struct AbilityStats;
	class Ability;
	class Collider;
	class Entity;
	class EntityAbilityController;
	class EntityAudioController;
	struct EntityModelState;
	struct EntityMovementState;
	class EntityPendingSpawnQueue;
	class EntityStatusController;
	class EntityVisualState;
	template <typename Derived>
	class EntityBuilder;
}

namespace Nawia::Item {
	class ItemDatabase;
}

namespace Nawia::Audio {
	class AudioManager;
}

namespace Nawia::Core {
	class ResourceManager;
}

namespace Nawia::Game {
	class CombatEventBus;
}

namespace Nawia::Entity {

	class Entity : public std::enable_shared_from_this<Entity> {
	public:
		Entity(
			const std::string& name,
			float start_x,
			float start_y,
			const std::shared_ptr<Texture2D>& texture,
			int max_hp);

		virtual ~Entity();

		virtual void update(float delta_time);
		virtual void render(const Camera3D& camera);

		// Identity and position.
		[[nodiscard]] float getX() const;
		[[nodiscard]] float getY() const;
		[[nodiscard]] Vector2 getPosition() const;
		[[nodiscard]] float getAltitude() const;
		void setX(float x);
		void setY(float y);
		void setPosition(Vector2 position);
		void translatePosition(float dx, float dy);
		void setAltitude(float altitude);
		void assignEntityId(EntityId entity_id);
		[[nodiscard]] EntityId getEntityId() const { return _entity_id; }
		[[nodiscard]] bool hasEntityId() const { return _entity_id != INVALID_ENTITY_ID; }
		[[nodiscard]] const std::string& getName() const { return _name; }
		void setName(const std::string& name) { _name = name; }
		[[nodiscard]] EntityType getType() const { return _type; }
		void setType(EntityType type) { _type = type; }

		// Casting and statuses.
		void beginCastTelemetry(std::string cast_name, float duration_seconds, bool interruptible);
		void clearCastTelemetry();
		[[nodiscard]] const EntityCastState& getCastState() const;
		[[nodiscard]] bool isCasting() const;
		virtual void applyRoot(float duration);
		void applyPoison(float duration, int damage_per_tick, float tick_interval = 1.0f);
		void applyPoison(
			float duration,
			int damage_per_tick,
			float tick_interval,
			const DamageSourceContext& source_context);
		void clearStatusEffects();
		[[nodiscard]] bool isMovementRooted() const;
		[[nodiscard]] bool isPoisoned() const;
		[[nodiscard]] float getRootRemaining() const;
		[[nodiscard]] float getPoisonRemaining() const;

		// Spatial queries and transforms.
		[[nodiscard]] Vector2 getCenter() const;
		[[nodiscard]] virtual Vector3 getWorldPos3D() const;
		[[nodiscard]] Vector2 getScreenPosition(const Camera3D& camera) const;
		[[nodiscard]] virtual bool isMouseOver(float screen_x, float screen_y, const Camera3D& camera) const;
		[[nodiscard]] BoundingBox getBoundingBox() const;
		[[nodiscard]] Matrix getWorldTransformMatrix() const;
		[[nodiscard]] bool checkRayHitsMesh(const Ray& ray) const;
		[[nodiscard]] RayCollision getRayMeshCollision(const Ray& ray) const;
		[[nodiscard]] virtual bool isVisibleInCamera(const Camera3D& camera, float screen_margin = 96.0f) const;
		[[nodiscard]] virtual bool isPerceptionVisible() const { return !_dormant && !isDead() && !isDying(); }

		// Movement.
		void setVelocity(float x, float y);
		[[nodiscard]] Vector2 getVelocity() const;
		void setScale(float scale);
		[[nodiscard]] float getScale() const;
		void setRotation(float angle);
		[[nodiscard]] float getRotation() const;
		virtual void moveTo(float x, float y);
		virtual void updateMovement(float dt);
		void stopMovement();
		void setMovementTarget(float x, float y);
		void stopMotion();
		void tickPathRecalcTimer(float dt);
		[[nodiscard]] bool isPathRecalcDue() const;
		void resetPathRecalcTimer(float interval);
		void clearPathRecalcTimer();
		void setMovementSpeed(float speed);
		[[nodiscard]] float getMovementSpeed() const;
		void setSpeedMultiplier(float multiplier);
		[[nodiscard]] float getSpeedMultiplier() const;
		void setDamageMultiplier(float multiplier);
		[[nodiscard]] float getDamageMultiplier() const;
		[[nodiscard]] bool isMoving() const;
		static constexpr float DEFAULT_PATH_RECALC_INTERVAL = 0.5f;
		void chaseTarget(float dt, float path_recalc_interval = DEFAULT_PATH_RECALC_INTERVAL);

		// Rendering and model visual state.
		[[nodiscard]] const std::shared_ptr<Texture2D>& getTexture() const { return _texture; }
		void setModelTint(Color tint);
		[[nodiscard]] Color getModelTint() const;
		void setHovered(bool hovered);
		void hideMeshIndex(int mesh_index);
		void setModelFacingOffset(float deg);
		[[nodiscard]] float getModelFacingOffset() const;
		void rotateTowards(float world_x, float world_y);
		void rotateTowardsCenter(float world_x, float world_y);

		// Model and animation resources.
		void loadModel(const std::string& path, bool rotate_model = false);
		void replaceModel(const std::string& path, bool rotate_model = false);
		void useSharedModel(const Model& model);
		bool alignLoadedModelToGround();
		void renderLoadedModel(Color tint = WHITE) const;
		bool fitLoadedModelToHeight(float target_height, bool center_xz = true, bool align_to_ground = true);
		void addAnimation(const std::string& name, const std::string& path);
		void addAnimation(const std::string& name, const std::string& path, int clip_index);
		void loadAnimationBundle(const std::string& path);
		static void preloadAnimationData(const std::string& path);
		static void setSharedResourceManager(Core::ResourceManager* manager);
		[[nodiscard]] static Core::ResourceManager* getSharedResourceManager();
		static void setCombatEventBus(Game::CombatEventBus* event_bus);
		[[nodiscard]] static Game::CombatEventBus* getCombatEventBus();
		static void setAudioListener(const std::shared_ptr<Entity>& listener);
		void playAnimation(
			const std::string& name,
			bool loop = true,
			bool lock_movement = false,
			int start_frame = 0,
			bool force = false);
		void playAnimationPingPong(
			const std::string& name,
			bool lock_movement = true,
			int start_frame = 0,
			bool force = false);
		void playAnimationFreezeOnLastFrame(
			const std::string& name,
			bool lock_movement = false,
			int start_frame = 0,
			bool force = true);
		void setAnimationSpeed(float multiplier);
		[[nodiscard]] float getAnimationSpeed() const;
		[[nodiscard]] int getAnimationFrameCount(const std::string& name) const;
		[[nodiscard]] bool hasAnimationReachedFrame(float frame) const;
		[[nodiscard]] bool hasAnimationReachedRatio(const std::string& animation_name, float frame_ratio) const;
		bool consumeAnimationFrameTrigger(const std::string& animation_name, float frame_ratio, bool& consumed) const;
		void holdAnimationFrame(const std::string& animation_name, int frame);
		void playAnimationReverseOnce(const std::string& animation_name, bool lock_movement = true);
		bool advanceAnimationTowardFrame(float dt, int target_frame, bool lock_when_reached = true);
		void applyCurrentAnimationFrame();
		[[nodiscard]] bool isAnimationLocked() const;
		static constexpr float ANIMATION_DURATION_SCALE = 1.5f;
		[[nodiscard]] Model& getModel();
		[[nodiscard]] bool hasModelLoaded() const;

		// Audio.
		void setAudioManager(Audio::AudioManager* audio_manager) { _audio_manager = audio_manager; }
		[[nodiscard]] Audio::AudioManager* getAudioManager() const { return _audio_manager; }
		void playSoundEffect(const std::string& id, float volume = 1.0f, bool restart_if_playing = true, float pitch = 1.0f) const;
		void stopSoundEffect(const std::string& id) const;

		// Combat state.
		virtual void takeDamage(int dmg);
		void takeDamage(int dmg, const DamageSourceContext& source_context);
		bool damageTargetInRange(float max_distance, int damage, std::string source_label);
		void die();
		[[nodiscard]] bool isDead() const { return _hp <= 0; }
		[[nodiscard]] bool isDying() const { return _is_dying; }
		[[nodiscard]] bool shouldPersistAfterDeath() const { return _persist_after_death; }
		void setPersistAfterDeath(bool value) { _persist_after_death = value; }
		[[nodiscard]] int getHP() const { return _hp; }
		[[nodiscard]] int getMaxHP() const { return _max_hp; }
		void setHP(int hp);
		void setMaxHp(int max_hp);
		void setMaxHpPreservingCurrentHp(int max_hp);
		void setDeathAnimationName(std::string animation_name);
		void setHealToFullOnKill(bool value) { _heal_to_full_on_kill = value; }
		[[nodiscard]] bool healsToFullOnKill() const { return _heal_to_full_on_kill; }

		// Collider.
		void setCollider(std::unique_ptr<Collider> collider);
		[[nodiscard]] Collider* getCollider() const { return _collider.get(); }
		static bool DebugColliders;

		// Abilities.
		static AbilityStats getAbilityStatsFromJson(const std::string& name);
		void addAbility(const std::shared_ptr<Ability>& ability);
		void setAbility(int index, const std::shared_ptr<Ability>& ability);
		[[nodiscard]] std::shared_ptr<Ability> getAbility(int index);
		[[nodiscard]] const std::vector<std::shared_ptr<Ability>>& getAbilities() const;
		void updateAbilities(float dt) const;

		// Spawns created during update.
		void addPendingSpawn(std::shared_ptr<Entity> entity);
		[[nodiscard]] const std::vector<std::shared_ptr<Entity>>& getPendingSpawns() const;
		void clearPendingSpawns();

		// Factions and targets.
		[[nodiscard]] Faction getFaction() const { return _faction; }
		void setFaction(Faction faction) { _faction = faction; }
		virtual void setTarget(const std::shared_ptr<Entity>& target) { _target = target; }
		[[nodiscard]] std::shared_ptr<Entity> getTarget() const { return _target.lock(); }
		[[nodiscard]] std::shared_ptr<Entity> getLiveTarget() const;
		bool faceTargetCenter();
		void rememberDamageSource(Entity* source, std::string source_label = {});
		void rememberDamageSource(DamageSourceContext source_context);
		[[nodiscard]] static DamageSourceContext makeDamageSourceContext(Entity* source, std::string source_label = {});
		[[nodiscard]] const DamageSourceContext& getLastDamageSourceContext() const { return _last_damage_source; }
		[[nodiscard]] std::shared_ptr<Entity> getLastDamageSource() const { return _last_damage_source.source.lock(); }
		[[nodiscard]] float getDistanceToTarget() const;
		[[nodiscard]] Vector2 getTargetPosition() const;
		[[nodiscard]] bool hasValidTarget() const;

		// Dormancy and persistence.
		void setDormant(bool dormant) { _dormant = dormant; }
		[[nodiscard]] bool isDormant() const { return _dormant; }
		[[nodiscard]] virtual bool shouldWakeOnLocationChange() const { return true; }
		[[nodiscard]] virtual nlohmann::json serializeState() const;
		virtual void applyState(const nlohmann::json& state, Item::ItemDatabase* item_database = nullptr);

	protected:
		template <typename T> friend class EntityBuilder;
		Entity();

		void updateAnimation(float dt);
		void updateStatusEffects(float dt);
		void updateCastTelemetry(float dt);
		void updateMovementSound(const std::string& path, bool should_play, float volume = 0.55f, float pitch = 1.0f);
		void unloadModelData();
		virtual void onDeathStarted() {}
		virtual void updateAttachedModelAnimation(const ModelAnimation& animation, int frame) {}
		virtual void drawAttachedModel(Vector3 pos3d, float visual_rotation) const {}

	private:
		std::unique_ptr<EntityModelState> _model_state;
		std::unique_ptr<EntityMovementState> _movement_state;
		EntityId _entity_id = INVALID_ENTITY_ID;
		std::shared_ptr<Texture2D> _texture;
		EntityType _type = EntityType::None;
		Audio::AudioManager* _audio_manager = nullptr;
		std::unique_ptr<Collider> _collider;

		int _hp = 1;
		int _max_hp = 1;
		bool _persist_after_death = false;
		bool _dormant = false;
		bool _heal_to_full_on_kill = false;
		bool _combat_death_event_emitted = false;
		bool _is_dying = false;
		std::string _death_anim_name = "death";

		std::weak_ptr<Entity> _target;
		DamageSourceContext _last_damage_source;
		Faction _faction = Faction::None;
		std::string _name;
		std::unique_ptr<EntityAbilityController> _ability_controller;
		std::unique_ptr<EntityAudioController> _audio_controller;
		std::unique_ptr<EntityPendingSpawnQueue> _pending_spawn_queue;
		std::unique_ptr<EntityStatusController> _status_controller;
		std::unique_ptr<EntityVisualState> _visual_state;
	};

} // namespace Nawia::Entity
