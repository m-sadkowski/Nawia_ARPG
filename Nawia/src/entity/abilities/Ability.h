#pragma once
#include "AbilityEffect.h"
#include "AbilityStats.h"

#include <raylib.h>
#include <functional>
#include <vector>
#include <string>
#include <memory>

namespace Nawia::Entity
{
	class Entity;
	
	/**
	 * @enum AbilityTargetType
	 * @brief Defines how an ability targets its effects.
	 */
	enum class AbilityTargetType {
		POINT, ///< Casted at a location (e.g., Fireball, Blink)
		UNIT,  ///< Casted on a specific target entity
		SELF   ///< Casted on the caster (e.g., heal, shield)
	};

	/**
	 * @class Ability
	 * @brief Base class for all abilities (skills) in the game.
	 * 
	 * An Ability is a logical object (not an Entity) that belongs to an Entity (the caster).
	 * Its main purpose is to create effects (projectiles, AoE, etc.) when used.
	 * 
	 * ## Creating a New Ability
	 * 
	 * 1. Create a class inheriting from `Ability`
	 * 2. Implement the `cast(target_x, target_y)` method to spawn your effect
	 * 3. The base class handles cooldowns automatically
	 * 
	 * ## Example
	 * 
	 * ```cpp
	 * class FireballAbility : public Ability {
	 * public:
	 *     FireballAbility(const std::shared_ptr<Texture2D>& proj_tex, const std::shared_ptr<Texture2D>& icon)
	 *         : Ability("Fireball", getStatsFromJson("fireball"), AbilityTargetType::POINT, icon),
	 *           _projectile_tex(proj_tex) {}
	 * 
	 *     std::unique_ptr<Entity> cast(float target_x, float target_y) override {
	 *         Vector2 start = _caster->getCollider()->getCenter();
	 *         auto projectile = std::make_unique<Projectile>(start.x, start.y, target_x, target_y, _projectile_tex, _stats);
	 *         projectile->setFaction(_caster->getFaction());
	 *         return projectile;
	 *     }
	 * private:
	 *     std::shared_ptr<Texture2D> _projectile_tex;
	 * };
	 * ```
	 * 
	 * @see AbilityEffect for creating visual effects spawned by abilities
	 * @see AbilityStats for configuration of damage, cooldown, range, etc.
	 */
	class Ability {
	public:
		/**
		 * @brief Construct a new Ability.
		 * @param name Display name of the ability
		 * @param stats Ability configuration (damage, cooldown, range, etc.)
		 * @param target_type How the ability targets (POINT, UNIT, SELF)
		 * @param icon_texture UI icon texture
		 */
		Ability(std::string name, const AbilityStats& stats, AbilityTargetType target_type, const std::shared_ptr<Texture2D>& icon_texture);

		virtual ~Ability() = default;

		/**
		 * @brief Update ability state (cooldowns, etc.)
		 * @param dt Delta time in seconds
		 */
		virtual void update(float dt);

		/**
		 * @brief Check if ability is ready to use (off cooldown).
		 * @return true if ready, false if on cooldown
		 */
		[[nodiscard]] bool isReady() const;
		
		/**
		 * @brief Cast the ability at a target location.
		 * 
		 * Override this method to define what happens when the ability is used.
		 * Return a new Entity (projectile, effect) to spawn it in the world,
		 * or nullptr if the ability has an instant effect with no visual entity.
		 * 
		 * @param target_x World X coordinate of target
		 * @param target_y World Y coordinate of target
		 * @return Spawned entity, or nullptr for instant effects
		 */
		virtual std::unique_ptr<Entity> cast(float target_x, float target_y) = 0;
		
		[[nodiscard]] std::string getName() const;
		[[nodiscard]] float getCastRange() const;
		[[nodiscard]] float getCooldownTimer() const { return _cooldown_timer; }
		[[nodiscard]] AbilityTargetType getTargetType() const;
		[[nodiscard]] const AbilityStats& getStats() const { return _stats; }
		

		[[nodiscard]] std::shared_ptr<Texture2D> getIcon() const { return _icon_texture; }
		
		void setCaster(Entity* caster) { _caster = caster; }
		[[nodiscard]] Entity* getCaster() const { return _caster; }

	protected:
		std::string _name;
		std::shared_ptr<Texture2D> _icon_texture;
		AbilityStats _stats;
		float _cooldown_timer;
		AbilityTargetType _target_type;
		Entity* _caster;  ///< Entity that owns this ability

		/// Start the cooldown timer (call after successful cast)
		void startCooldown() { _cooldown_timer = _stats.cooldown; }
	};

} // namespace Nawia::Entity