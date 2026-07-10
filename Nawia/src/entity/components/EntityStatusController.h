#pragma once

#include <Entity.h>

#include <string>
#include <vector>

namespace Nawia::Entity {

	class EntityStatusController {
	public:
		struct PoisonTick {
			int damage = 0;
			DamageSourceContext source;
		};

		void beginCast(std::string cast_name, float duration_seconds, bool interruptible);
		void clearCast();
		void updateCast(float dt);
		[[nodiscard]] const EntityCastState& castState() const { return _cast_state; }
		[[nodiscard]] bool isCasting() const { return _cast_state.active; }

		void applyRoot(float duration);
		[[nodiscard]] bool updateRoot(float dt);
		[[nodiscard]] bool isMovementRooted() const { return _root_timer > 0.0f; }
		[[nodiscard]] float rootRemaining() const { return _root_timer; }

		void applyPoison(
			float duration,
			int damage_per_tick,
			float tick_interval,
			const DamageSourceContext& source_context);
		[[nodiscard]] std::vector<PoisonTick> updatePoison(float dt, bool can_apply_damage);
		[[nodiscard]] bool isPoisoned() const { return _poison_timer > 0.0f; }
		[[nodiscard]] float poisonRemaining() const { return _poison_timer; }

		void clearStatusEffects();

	private:
		float _root_timer = 0.0f;
		float _poison_timer = 0.0f;
		float _poison_tick_timer = 0.0f;
		float _poison_tick_interval = 1.0f;
		int _poison_damage_per_tick = 0;
		DamageSourceContext _poison_damage_source;
		EntityCastState _cast_state;
	};

}
