#pragma once

#include <Entity.h>

#include <map>
#include <memory>
#include <string>

namespace Nawia::Entity {

	enum class BossHazardPhase {
		Warning,
		Active,
		Expired
	};

	struct BossTelegraphHazardConfig {
		std::string name = "Boss Hazard";
		Vector2 position = {0.0f, 0.0f};
		float altitude = 0.0f;
		float radius = 2.0f;
		float warning_seconds = 1.0f;
		float active_seconds = 3.0f;
		int damage_per_tick = 10;
		float tick_interval = 0.75f;
		float root_seconds_on_hit = 0.0f;
		DamageSourceContext source_context;
		Color warning_color = {255, 90, 40, 255};
		Color active_color = {220, 35, 20, 255};
	};

	/**
	 * @brief Tactical boss area with a warning phase and an active damage phase.
	 *
	 * The hazard is a world entity so rendering, telemetry, and agent perception
	 * all see the same object. It does not block movement or pathfinding; future
	 * agent logic should decide how to avoid it.
	 */
	class BossTelegraphHazard : public Entity {
	public:
		explicit BossTelegraphHazard(BossTelegraphHazardConfig config);

		void update(float dt) override;
		void render(const Camera3D& camera) override;
		[[nodiscard]] bool isMouseOver(float screen_x, float screen_y, const Camera3D& camera) const override;
		[[nodiscard]] bool isVisibleInCamera(const Camera3D& camera, float screen_margin = 96.0f) const override;
		[[nodiscard]] bool isPerceptionVisible() const override;

		[[nodiscard]] BossHazardPhase getHazardPhase() const;
		[[nodiscard]] const char* getHazardPhaseName() const;
		[[nodiscard]] bool isWarning() const;
		[[nodiscard]] bool isActiveHazard() const;
		[[nodiscard]] bool isExpired() const;
		[[nodiscard]] float getRadius() const { return _radius; }
		[[nodiscard]] float getWarningSeconds() const { return _warning_seconds; }
		[[nodiscard]] float getActiveSeconds() const { return _active_seconds; }
		[[nodiscard]] float getElapsedSeconds() const { return _elapsed_seconds; }
		[[nodiscard]] float getTimeToActivate() const;
		[[nodiscard]] float getRemainingActiveSeconds() const;
		[[nodiscard]] int getDamagePerTick() const { return _damage_per_tick; }
		[[nodiscard]] float getTickInterval() const { return _tick_interval; }
		[[nodiscard]] EntityId getSourceEntityId() const { return _source_context.source_id; }
		[[nodiscard]] const std::string& getSourceLabel() const { return _source_context.label; }

		void applyToTarget(const std::shared_ptr<Entity>& target);

	private:
		[[nodiscard]] bool canAffectTarget(const std::shared_ptr<Entity>& target) const;
		[[nodiscard]] bool isTargetInside(const Entity& target) const;
		[[nodiscard]] Color currentFillColor() const;
		[[nodiscard]] Color currentEdgeColor() const;
		[[nodiscard]] DamageSourceContext damageContext() const;

		float _radius = 2.0f;
		float _warning_seconds = 1.0f;
		float _active_seconds = 3.0f;
		int _damage_per_tick = 10;
		float _tick_interval = 0.75f;
		float _root_seconds_on_hit = 0.0f;
		float _elapsed_seconds = 0.0f;
		DamageSourceContext _source_context;
		Color _warning_color = {255, 90, 40, 255};
		Color _active_color = {220, 35, 20, 255};
		std::map<EntityId, float> _next_tick_time_by_target;
	};

} // namespace Nawia::Entity
