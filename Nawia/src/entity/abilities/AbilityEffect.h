#pragma once
#include "Entity.h"
#include "AbilityStats.h"

#include <vector>

namespace Nawia::Entity {

	/**
	 * @class AbilityEffect
	 * @brief Visual effect entity spawned by abilities (projectiles, explosions, AoE zones).
	 * 
	 * AbilityEffect extends Entity with ability-specific features:
	 * - Built-in lifetime management (expires after `duration`)
	 * - Collision handling for combat (damage on hit)
	 * - Hit tracking to prevent multiple hits on the same target
	 * 
	 * ## Creating a New Effect
	 * 
	 * 1. Create a class inheriting from `AbilityEffect`
	 * 2. Override `update()` for movement/behavior (call base method!)
	 * 3. Override `onCollision()` to define hit effects
	 * 
	 * ## Example: Fireball Projectile
	 * 
	 * ```cpp
	 * class FireballEffect : public AbilityEffect {
	 * public:
	 *     FireballEffect(float x, float y, const std::shared_ptr<Texture2D>& tex, 
	 *                    const AbilityStats& stats, float target_x, float target_y)
	 *         : AbilityEffect("Fireball", x, y, tex, stats)
	 *     {
	 *         float angle = atan2(target_y - y, target_x - x);
	 *         setVelocity(cos(angle) * stats.projectile_speed, sin(angle) * stats.projectile_speed);
	 *         setCollider(std::make_unique<CircleCollider>(this, stats.hitbox_radius, 0.0f, 0.0f));
	 *     }
	 * 
	 *     void onCollision(const std::shared_ptr<Entity>& target) override {
	 *         if (hasHit(target)) return;
	 *         target->takeDamage(_stats.damage);
	 *         addHit(target);
	 *         die(); // Destroy on hit
	 *     }
	 * };
	 * ```
	 * 
	 * @see Ability for the ability that spawns effects
	 */
	class AbilityEffect : public Entity {
	public:
		/**
		 * @brief Construct a new AbilityEffect.
		 * @param name Effect name
		 * @param x Starting world X position
		 * @param y Starting world Y position
		 * @param tex Texture for the effect
		 * @param stats Ability stats (damage, duration, hitbox, etc.)
		 */
		AbilityEffect(const std::string& name, float x, float y, const std::shared_ptr<Texture2D>& tex, const AbilityStats& stats);

		/**
		 * @brief Update effect state.
		 * Base implementation handles movement and lifetime.
		 * Override for custom behavior, but always call base method.
		 */
		void update(float dt) override;
		
		/**
		 * @brief Check if effect has expired (lifetime elapsed).
		 * @return true if effect should be removed
		 */
		[[nodiscard]] bool isExpired() const;
		
		[[nodiscard]] int getDamage() const;
		[[nodiscard]] const AbilityStats& getStats() const { return _stats; }

		/**
		 * @brief Check collision with a target entity.
		 * Default implementation uses colliders and faction filtering.
		 * @param target Entity to check collision with
		 * @return true if collision detected
		 */
		[[nodiscard]] virtual bool checkCollision(const std::shared_ptr<Entity>& target) const;
		
		/**
		 * @brief Called when collision is detected.
		 * Override to implement damage, effects, or destruction.
		 * @param target Entity that was hit
		 */
		virtual void onCollision(const std::shared_ptr<Entity>& target);
		
		/**
		 * @brief Check if target was already hit by this effect.
		 * Use to prevent multiple hits per target.
		 */
		[[nodiscard]] bool hasHit(const std::shared_ptr<Entity>& target) const;
		
		/// Record a target as having been hit
		void addHit(const std::shared_ptr<Entity>& target);

	protected:
		AbilityStats _stats;
		float _timer;  ///< Lifetime counter
		std::vector<std::weak_ptr<Entity>> _hit_entities;  ///< Entities already hit
	};

} // namespace Nawia::Entity