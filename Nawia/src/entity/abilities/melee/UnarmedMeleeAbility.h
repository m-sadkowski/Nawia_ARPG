#pragma once

#include <Ability.h>
#include <UnarmedMeleeEffect.h>

#include <memory>
#include <string>

namespace Nawia::Entity {

	class UnarmedMeleeAbility : public Ability {
	public:
		UnarmedMeleeAbility(
			std::string ability_name,
			const std::string& stats_key,
			std::string animation_name,
			AbilityTargetType target_type,
			bool direct_target_hit,
			UnarmedMeleeEffect::Shape effect_shape,
			float spawn_ratio,
			float hitbox_width,
			float knockback_distance,
			bool ping_pong_animation,
			const std::shared_ptr<Texture2D>& icon_tex);

		void update(float dt) override;
		AbilitySpawn cast(float target_x, float target_y) override;

	private:
		[[nodiscard]] float calculateAnimationDuration() const;
		void spawnEffect();

		std::string _animation_name;
		bool _direct_target_hit = false;
		UnarmedMeleeEffect::Shape _effect_shape = UnarmedMeleeEffect::Shape::Cone;
		float _spawn_ratio = 0.45f;
		float _hitbox_width = 55.0f;
		float _knockback_distance = 0.0f;
		bool _ping_pong_animation = true;
		bool _is_active = false;
		bool _has_spawned = false;
		float _active_time = 0.0f;
		float _spawn_delay = 0.0f;
	};

} // namespace Nawia::Entity
