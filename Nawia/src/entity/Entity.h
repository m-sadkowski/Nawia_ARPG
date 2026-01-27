#pragma once
#include "AbilityStats.h"

#include <MathUtils.h>

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
		Projectile, // AbilityEffect
		Trigger, // Checkpoint
		Chest, // Interactable
		Item
	};

	/**
	 * @class Entity
	 * @brief Base class for all game objects in the world (players, enemies, projectiles, items).
	 * 
	 * Entity provides core functionality shared by all game objects:
	 * - Position and movement in isometric world coordinates
	 * - Health and damage system
	 * - 3D model rendering with animation support
	 * - Collision detection via attached Collider
	 * - Ability system for casting spells/attacks
	 * - Faction system for friend/foe detection
	 * 
	 * @note All coordinates are in isometric world space. Use getScreenPos() to convert
	 *       to screen coordinates for rendering or mouse interaction.
	 * 
	 * Usage example - Creating a simple entity:
	 * @code
	 *     auto texture = resourceManager.getTexture("player.png");
	 *     auto player = std::make_shared<Entity>("Player", 10.0f, 10.0f, texture, 100);
	 *     player->setFaction(Entity::Faction::Player);
	 *     player->setCollider(std::make_unique<CircleCollider>(0.5f));
	 *     player->addAbility(std::make_shared<FireballAbility>(fireballTex));
	 * @endcode
	 * 
	 * Usage example - Subclassing:
	 * @code
	 *     class Enemy : public Entity {
	 *     public:
	 *         Enemy(float x, float y, std::shared_ptr<Texture2D> tex)
	 *             : Entity("Enemy", x, y, tex, 50) {
	 *             setFaction(Faction::Enemy);
	 *         }
	 *         
	 *         void update(float dt) override {
	 *             Entity::update(dt);
	 *             // Custom AI logic here
	 *         }
	 *     };
	 * @endcode
	 */
	class Entity {
	public:
		/**
		 * @brief Construct a new Entity.
		 * @param name Display name of the entity
		 * @param start_x Initial X position in world coordinates
		 * @param start_y Initial Y position in world coordinates
		 * @param texture Shared texture for rendering
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
		 * @brief Render the entity at its current position.
		 * @param offset_x Camera X offset for screen positioning
		 * @param offset_y Camera Y offset for screen positioning
		 */
		virtual void render(float offset_x, float offset_y);


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
		/// @brief Helpers for converting between world (isometric) and screen coordinates.
		/// @{
		
		/** @brief Convert world position to screen pixel coordinates. */
		[[nodiscard]] Vector2 getScreenPos(float mouse_x, float mouse_y, float cam_x, float cam_y) const;
		
		/** @brief Convert world position to raw isometric coordinates (before camera offset). */
		[[nodiscard]] Vector2 getIsoPos(float world_x, float world_y, float cam_x, float cam_y) const;
		
		/** @brief Check if mouse is hovering over this entity. */
		[[nodiscard]] virtual bool isMouseOver(float mouse_x, float mouse_y, float cam_x, float cam_y) const;
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
		
		[[nodiscard]] int getAnimationFrameCount(const std::string& name) const;
		[[nodiscard]] bool isAnimationLocked() const { return _anim_locked; }

		void setRotation(const float angle) { _rotation = angle; }
		[[nodiscard]] float getRotation() const { return _rotation; }
		
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
		
		/**
		 * @brief Queue an entity to be spawned after the update loop.
		 * 
		 * Use this instead of direct spawning to avoid modifying the entity list
		 * during iteration. EntityManager processes pending spawns each frame.
		 * 
		 * @param entity Entity to spawn (e.g., projectile from ability)
		 */
		void addPendingSpawn(const std::shared_ptr<Entity>& entity) { _pending_spawns.push_back(entity); }
		[[nodiscard]] std::vector<std::shared_ptr<Entity>> getPendingSpawns() { return _pending_spawns; }
		void clearPendingSpawns() { _pending_spawns.clear(); }


		// ═══════════════════════════════════════════════════════════════════════
		// FACTION SYSTEM
		// ═══════════════════════════════════════════════════════════════════════
		
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

		[[nodiscard]] Faction getFaction() const { return _faction; }
		void setFaction(Faction faction) { _faction = faction; }


		EntityType getType() const { return _type; }
		void setType(EntityType type) { _type = type; }

	protected:
		Vector2 _pos;
		Vector2 _velocity;
		float _scale;
		std::shared_ptr<Texture2D> _texture;
		EntityType _type = EntityType::None;
		
		std::unique_ptr<Collider> _collider;
		
		std::vector<std::shared_ptr<Entity>> _pending_spawns;

		int _hp;
		int _max_hp;

		// 3D Model & Animation Data
		Model _model;
		std::vector<ModelAnimation> _animations;
		std::map<std::string, int> _animation_map;
		
		int _current_anim_index;
		int _anim_frame_counter;
		float _rotation;
		bool _model_loaded;
		bool _anim_looping;
		bool _anim_locked;
		bool _hovered;

		Faction _faction;

		std::string _name;

		// 3D Rendering Support
		RenderTexture2D _target;
		Camera3D _camera;
		bool _use_3d_rendering;

		void updateAnimation(float dt);
		
		std::vector<std::shared_ptr<Ability>> _abilities;
	};

} // namespace Nawia::Entity