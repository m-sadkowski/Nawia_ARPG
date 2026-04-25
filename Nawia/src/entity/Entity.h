#pragma once

#include "AbilityStats.h"

#include <raylib.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace Nawia::Entity {
	class Ability;
	class Collider;

	enum class EntityType {
		None,
		Player,
		Enemy,
		Ally,
		NPCStatic,
		Projectile, // AbilityEffect
		Trigger, // Checkpoint
		Chest, // Interactable
		Item
	};

	/**
	* @enum Faction
	* @brief Determines friend/foe relationships for combat and AI.
	*/
	enum class Faction {
		Player,   ///< Player-controlled entities
		Enemy,    ///< Hostile to player
		Neutral,  ///< Non-combatant
		Ally,     ///< Friendly to player
		None      ///< No faction (e.g., projectiles inherit caster's faction)
	};

	/**
	 * @class Entity
	 * @brief Base class for all game objects in the world (players, enemies, projectiles, items).
	 * 
	 * Entity provides core functionality shared by all game objects:
	 * - Position and movement in world coordinates (XZ plane, Y=0)
	 * - Health and damage system
	 * - 3D model rendering with animation support
	 * - Collision detection via attached Collider
	 * - Ability system for casting spells/attacks
	 * - Faction system for friend/foe detection
	 * 
	 * @note Positions use Vector2{x, y} internally, which maps to 3D world as {x, 0, y}.
	 *       getX() corresponds to world X, getY() corresponds to world Z.
	 */
	class Entity : public std::enable_shared_from_this<Entity> {
	public:
		/**
		 * @brief Construct a new Entity.
		 * @param name Display name of the entity
		 * @param start_x Initial X position in world coordinates
		 * @param start_y Initial Y (Z in 3D) position in world coordinates
		 * @param texture Shared texture for rendering (fallback 2D)
		 * @param max_hp Maximum (and initial) health points
		 */
		Entity(const std::string& name, float start_x, float start_y, const std::shared_ptr<Texture2D>& texture, int max_hp);
		
		virtual ~Entity();

		/**
		 * @brief Update entity state each frame. Override for custom behavior.
		 * @param delta_time Time elapsed since last frame (seconds)
		 */
		virtual void update(float delta_time);
		
		/**
		 * @brief Render the entity in 3D space.
		 * Must be called between BeginMode3D/EndMode3D.
		 * @param camera The 3D camera (for debug rendering, hover effects, etc.)
		 */
		virtual void render(const Camera3D& camera);


		// ═══════════════════════════════════════════════════════════════════════
		// POSITION & COORDINATES
		// ═══════════════════════════════════════════════════════════════════════
		
		/// @name Position Accessors
		/// @{
		[[nodiscard]] float getX() const { return _pos.x; }
		[[nodiscard]] float getY() const { return _pos.y; }
		void setX(float x) { _pos.x = x; }
		void setY(float y) { _pos.y = y; }
		[[nodiscard]] Vector2 getCenter() const;
		/// @}

		/// @name Coordinate Conversion
		/// @{
		
		/** @brief Get 3D world position (on ground plane). Override for altitude. */
		[[nodiscard]] virtual Vector3 getWorldPos3D() const { return { _pos.x, 0.0f, _pos.y }; }

		/** @brief Project entity world position to screen coordinates. */
		[[nodiscard]] Vector2 getScreenPosition(const Camera3D& camera) const;
		
		/** @brief Check if mouse ray hits the entity's 3D bounding box. */
		[[nodiscard]] virtual bool isMouseOver(float screen_x, float screen_y, const Camera3D& camera) const;
		
		/** @brief Get world-space axis-aligned bounding box of the 3D model. */
		[[nodiscard]] BoundingBox getBoundingBox() const;

		/** @brief Build the world transform matrix used for rendering & collision. */
		[[nodiscard]] Matrix getWorldTransformMatrix() const;

		/** @brief Test if a ray hits this entity's 3D mesh (triangle-level). 
		 *  @return true if any mesh triangle is hit */
		[[nodiscard]] bool checkRayHitsMesh(const Ray& ray) const;

		/** @brief Test if a ray hits this entity's 3D mesh and return collision info. */
		[[nodiscard]] RayCollision getRayMeshCollision(const Ray& ray) const;
		/// @}


		// ═══════════════════════════════════════════════════════════════════════
		// TRANSFORM & PHYSICS
		// ═══════════════════════════════════════════════════════════════════════
		
		void setVelocity(const float x, const float y) { _velocity.x = x; _velocity.y = y; }
		[[nodiscard]] Vector2 getVelocity() const { return _velocity; }
		void setScale(const float scale) { _scale = scale; }
		[[nodiscard]] float getScale() const { return _scale; }
		void setHovered(const bool hovered) { _hovered = hovered; }

		// ═══════════════════════════════════════════════════════════════════════
		// HEALTH & DAMAGE
		// ═══════════════════════════════════════════════════════════════════════
		
		/**
		 * @brief Apply damage to this entity. Override for custom damage handling.
		 * @param dmg Amount of damage to apply
		 */
		virtual void takeDamage(int dmg);
		
		/** @brief Trigger death sequence for this entity. */
		void die();
		
		[[nodiscard]] bool isDead() const { return _hp <= 0; }
		[[nodiscard]] int getHP() const { return _hp; }
		[[nodiscard]] int getMaxHP() const { return _max_hp; }
		[[nodiscard]] std::string getName() const { return _name; }
		void setName(const std::string& name) { _name = name; }


		// ═══════════════════════════════════════════════════════════════════════
		// ANIMATION & 3D MODEL
		// ═══════════════════════════════════════════════════════════════════════
		
		/** @brief Load a 3D model from file. */
		void loadModel(const std::string& path, bool rotate_model = false);
		
		/** @brief Register an animation from file. */
		void addAnimation(const std::string& name, const std::string& path);
		
		/** 
		 * @brief Play a registered animation.
		 * @param name Animation name (must be registered via addAnimation)
		 * @param loop Whether to loop the animation
		 * @param lock_movement If true, prevents movement during animation
		 */
		void playAnimation(const std::string& name, bool loop = true, bool lock_movement = false, int start_frame = 0, bool force = false);

		void setAnimationSpeed(float multiplier) { _anim_speed_multiplier = multiplier; }
		float getAnimationSpeed() const { return _anim_speed_multiplier; }
		
		[[nodiscard]] int getAnimationFrameCount(const std::string& name) const;
		[[nodiscard]] bool isAnimationLocked() const { return _anim_locked; }

		void setRotation(const float angle) { _rotation = angle; }
		[[nodiscard]] float getRotation() const { return _rotation; }

		/** @brief Visual offset (degrees) added ONLY when rendering the model.
		 *  Use this to align the model's "front" with the math direction.
		 *  Does NOT affect getRotation() or ability directions. */
		void setModelFacingOffset(const float deg) { _model_facing_offset = deg; }
		
		/** @brief Rotate entity to face world coordinates. */
		void rotateTowards(float world_x, float world_y);


		// ═══════════════════════════════════════════════════════════════════════
		// COLLISION SYSTEM
		// ═══════════════════════════════════════════════════════════════════════
		
		/** @brief Attach a collider for collision detection. */
		void setCollider(std::unique_ptr<Collider> collider);
		
		/** @brief Rotate to face world coordinates using collider center as pivot. */
		void rotateTowardsCenter(float world_x, float world_y);
		
		[[nodiscard]] Collider* getCollider() const { return _collider.get(); }
		
		/** @brief Global debug flag - when true, all colliders are rendered. */
		static bool DebugColliders;


		// ═══════════════════════════════════════════════════════════════════════
		// ABILITY SYSTEM
		// ═══════════════════════════════════════════════════════════════════════
		
		/** @brief Load ability stats from abilities.json by name. */
		static AbilityStats getAbilityStatsFromJson(const std::string& name);
		
		/** @brief Add an ability to this entity's ability list. */
		void addAbility(const std::shared_ptr<Ability>& ability);
		
		/** @brief Get ability by slot index (0-based). */
		[[nodiscard]] std::shared_ptr<Ability> getAbility(int index);
		
		[[nodiscard]] const std::vector<std::shared_ptr<Ability>>& getAbilities() const { return _abilities; }
		
		/** @brief Update cooldowns and state for all abilities. */
		void updateAbilities(float dt) const;


		// ═══════════════════════════════════════════════════════════════════════
		// ENTITY SPAWNING
		// ═══════════════════════════════════════════════════════════════════════
		
		void addPendingSpawn(const std::shared_ptr<Entity>& entity) { _pending_spawns.push_back(entity); }
		[[nodiscard]] std::vector<std::shared_ptr<Entity>> getPendingSpawns() { return _pending_spawns; }
		void clearPendingSpawns() { _pending_spawns.clear(); }


		// ═══════════════════════════════════════════════════════════════════════
		// FACTION SYSTEM
		// ═══════════════════════════════════════════════════════════════════════

		[[nodiscard]] Faction getFaction() const { return _faction; }
		void setFaction(Faction faction) { _faction = faction; }


		EntityType getType() const { return _type; }
		void setType(EntityType type) { _type = type; }


		// ═══════════════════════════════════════════════════════════════════════
		// DORMANT SYSTEM
		// ═══════════════════════════════════════════════════════════════════════

		/**
		 * @brief Set dormant state. Dormant entities are invisible, frozen,
		 *        and excluded from collisions/interactions.
		 *
		 * Used by SpawnManager to pre-load entities at level start and
		 * activate them when the player approaches (proximity trigger).
		 */
		void setDormant(bool dormant) { _dormant = dormant; }
		[[nodiscard]] bool isDormant() const { return _dormant; }

	protected:
		template <typename T> friend class EntityBuilder;
		Entity() = default;

		Vector2 _pos = {0.0f, 0.0f};
		Vector2 _velocity = {0.0f, 0.0f};
		float _scale = 1.0f;
		std::shared_ptr<Texture2D> _texture;
		EntityType _type = EntityType::None;
		
		std::unique_ptr<Collider> _collider;
		
		std::vector<std::shared_ptr<Entity>> _pending_spawns;

		int _hp = 1;
		int _max_hp = 1;

		// 3D Model & Animation Data
		Model _model;
		std::vector<ModelAnimation> _animations;
		std::map<std::string, int> _animation_map;
		
		int _current_anim_index = 0;
		float _anim_frame_counter = 0.0f;
		float _anim_speed_multiplier = 1.0f;
		float _anim_fps = 30.0f;
		float _rotation = 0.0f;
		float _model_facing_offset = 90.0f; // visual offset for model facing vs math angle
		bool _model_loaded = false;
		bool _anim_looping = true;
		bool _anim_locked = false;
		bool _hovered = false;
		bool _dormant = false;  ///< When true, entity is invisible and frozen

		Faction _faction = Faction::None;

		std::string _name;

		void updateAnimation(float dt);
		
		std::vector<std::shared_ptr<Ability>> _abilities;
	};

	template <typename Derived>
	class EntityBuilder {
	public:
		EntityBuilder() = default;

		Derived& setName(const std::string& name) {
			_entity->_name = name;
			return self();
		}

		Derived& setTexture(const std::shared_ptr<Texture> texture) {
			_entity->_texture = texture;
			return self();
		}

		Derived& setPosition(const Vector2 pos) {
			_entity->_pos = pos;
			return self();
		}

		Derived& setX(const float x) {
			_entity->_pos.x = x;
			return self();
		}

		Derived& setY(const float y) {
			_entity->_pos.y = y;
			return self();
		}

		Derived& setMaxHp(const int max_hp) {
			_entity->_max_hp = max_hp;
			_entity->_hp = max_hp;
			return self();
		}
	protected:
		Entity* _entity = nullptr;

		Derived& self() {
			return static_cast<Derived&>(*this);
		}
	};

} // namespace Nawia::Entity