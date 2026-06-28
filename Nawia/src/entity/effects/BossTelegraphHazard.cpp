#include "BossTelegraphHazard.h"

#include <WorldAreaIndicator.h>

#include <raymath.h>

#include <algorithm>
#include <cmath>
#include <utility>

namespace Nawia::Entity {

	namespace Renderer = Nawia::Core::System::Renderer;

	namespace {
		[[nodiscard]] bool isCombatActorType(const EntityType type) {
			return type == EntityType::Player ||
				   type == EntityType::Ally ||
				   type == EntityType::Enemy;
		}

		[[nodiscard]] bool isPlayerSide(const Faction faction) {
			return faction == Faction::Player || faction == Faction::Ally;
		}
	}

	BossTelegraphHazard::BossTelegraphHazard(BossTelegraphHazardConfig config)
		: Entity(config.name, config.position.x, config.position.y, nullptr, 1),
		  _radius(std::max(0.1f, config.radius)),
		  _warning_seconds(std::max(0.0f, config.warning_seconds)),
		  _active_seconds(std::max(0.05f, config.active_seconds)),
		  _damage_per_tick(std::max(0, config.damage_per_tick)),
		  _tick_interval(std::max(0.05f, config.tick_interval)),
		  _root_seconds_on_hit(std::max(0.0f, config.root_seconds_on_hit)),
		  _source_context(std::move(config.source_context)),
		  _warning_color(config.warning_color),
		  _active_color(config.active_color) {
		setType(EntityType::Hazard);
		setFaction(_source_context.valid ? _source_context.source_faction : Faction::Enemy);
		setAltitude(config.altitude);
	}

	void BossTelegraphHazard::update(const float dt) {
		Entity::update(dt);
		_elapsed_seconds += std::max(0.0f, dt);
		if (isExpired())
			setHP(0);
	}

	void BossTelegraphHazard::render([[maybe_unused]] const Camera3D& camera) {
		if (isDead() || isDormant())
			return;

		const Vector3 center = {getCenter().x, getAltitude() + 0.08f, getCenter().y};
		Renderer::GroundDiscStyle disc_style;
		disc_style.radius = _radius;
		disc_style.height = isActiveHazard() ? 0.18f : 0.11f;
		disc_style.fill_color = currentFillColor();
		disc_style.core_color = currentEdgeColor();
		Renderer::drawSoftGroundDisc(center, disc_style);
	}

	bool BossTelegraphHazard::isMouseOver(
		[[maybe_unused]] const float screen_x,
		[[maybe_unused]] const float screen_y,
		[[maybe_unused]] const Camera3D& camera) const {
		return false;
	}

	bool BossTelegraphHazard::isVisibleInCamera(
		[[maybe_unused]] const Camera3D& camera,
		[[maybe_unused]] const float screen_margin) const {
		return !isDormant() && !isDead();
	}

	bool BossTelegraphHazard::isPerceptionVisible() const {
		return !isDormant() && !isDead();
	}

	BossHazardPhase BossTelegraphHazard::getHazardPhase() const {
		if (isExpired())
			return BossHazardPhase::Expired;
		if (isActiveHazard())
			return BossHazardPhase::Active;
		return BossHazardPhase::Warning;
	}

	const char* BossTelegraphHazard::getHazardPhaseName() const {
		switch (getHazardPhase()) {
			case BossHazardPhase::Warning:
				return "Warning";
			case BossHazardPhase::Active:
				return "Active";
			case BossHazardPhase::Expired:
				return "Expired";
		}
		return "Unknown";
	}

	bool BossTelegraphHazard::isWarning() const {
		return !isExpired() && _elapsed_seconds < _warning_seconds;
	}

	bool BossTelegraphHazard::isActiveHazard() const {
		return !isExpired() && _elapsed_seconds >= _warning_seconds;
	}

	bool BossTelegraphHazard::isExpired() const {
		return _elapsed_seconds >= _warning_seconds + _active_seconds;
	}

	float BossTelegraphHazard::getTimeToActivate() const {
		return std::max(0.0f, _warning_seconds - _elapsed_seconds);
	}

	float BossTelegraphHazard::getRemainingActiveSeconds() const {
		if (isExpired())
			return 0.0f;
		if (isWarning())
			return _active_seconds;
		return std::max(0.0f, _warning_seconds + _active_seconds - _elapsed_seconds);
	}

	void BossTelegraphHazard::applyToTarget(const std::shared_ptr<Entity>& target) {
		if (!isActiveHazard() || _damage_per_tick <= 0 || !canAffectTarget(target) || !isTargetInside(*target))
			return;

		const EntityId target_id = target->getEntityId();
		const float next_allowed_tick = _next_tick_time_by_target[target_id];
		if (_elapsed_seconds + 0.0001f < next_allowed_tick)
			return;

		target->takeDamage(_damage_per_tick, damageContext());
		if (_root_seconds_on_hit > 0.0f && !target->isDead() && !target->isDying())
			target->applyRoot(_root_seconds_on_hit);

		_next_tick_time_by_target[target_id] = _elapsed_seconds + _tick_interval;
	}

	bool BossTelegraphHazard::canAffectTarget(const std::shared_ptr<Entity>& target) const {
		if (!target || target.get() == this || target->isDead() || target->isDying() || target->isDormant())
			return false;

		if (!isCombatActorType(target->getType()))
			return false;

		const Faction source_faction = _source_context.valid ? _source_context.source_faction : getFaction();
		const Faction target_faction = target->getFaction();
		if (source_faction == Faction::Enemy)
			return isPlayerSide(target_faction);
		if (isPlayerSide(source_faction))
			return target_faction == Faction::Enemy;

		return false;
	}

	bool BossTelegraphHazard::isTargetInside(const Entity& target) const {
		return Vector2DistanceSqr(getCenter(), target.getCenter()) <= _radius * _radius;
	}

	Color BossTelegraphHazard::currentFillColor() const {
		if (isActiveHazard()) {
			const float pulse = 0.75f + 0.25f * std::sin(_elapsed_seconds * 12.0f);
			return Fade(_active_color, 0.42f * pulse);
		}

		const float warning_fraction = _warning_seconds <= 0.0f
			? 1.0f
			: std::clamp(_elapsed_seconds / _warning_seconds, 0.0f, 1.0f);
		return Fade(_warning_color, 0.16f + 0.24f * warning_fraction);
	}

	Color BossTelegraphHazard::currentEdgeColor() const {
		if (isActiveHazard())
			return Fade(_active_color, 0.54f);

		const float warning_fraction = _warning_seconds <= 0.0f
			? 1.0f
			: std::clamp(_elapsed_seconds / _warning_seconds, 0.0f, 1.0f);
		return Fade(_warning_color, 0.30f + 0.24f * warning_fraction);
	}

	DamageSourceContext BossTelegraphHazard::damageContext() const {
		DamageSourceContext context = _source_context;
		context.label = getName();
		return context;
	}

} // namespace Nawia::Entity
