#include "ProjectileAbility.h"

#include <Entity.h>
#include <Projectile.h>

#include <utility>

namespace Nawia::Entity {

	ProjectileAbility::ProjectileAbility(std::string ability_name,
										 const std::string& stats_key,
										 const AbilityTargetType target_type,
										 std::string projectile_name,
										 std::string model_path,
										 const float model_scale,
										 const std::shared_ptr<Texture2D>& hit_texture,
										 const std::shared_ptr<Texture2D>& icon_texture,
										 const float facing_offset)
		: Ability(std::move(ability_name), Entity::getAbilityStatsFromJson(stats_key), target_type, icon_texture),
		  _projectile_name(std::move(projectile_name)),
		  _model_path(std::move(model_path)),
		  _model_scale(model_scale),
		  _hit_texture(hit_texture),
		  _facing_offset(facing_offset) {}

	AbilitySpawn ProjectileAbility::cast(const float target_x, const float target_y) {
		if (!beginCast())
			return nullptr;

		const Vector2 spawn_position = getSpawnPosition();
		return std::make_shared<Projectile>(
			_projectile_name,
			spawn_position.x,
			spawn_position.y,
			target_x,
			target_y,
			_model_path,
			_model_scale,
			_stats,
			_caster,
			_hit_texture,
			_facing_offset);
	}

	Vector2 ProjectileAbility::getSpawnPosition() const {
		return getCasterCenter();
	}

	Vector2 ProjectileAbility::getCasterPosition() const {
		if (!_caster)
			return {0.0f, 0.0f};

		return {_caster->getX(), _caster->getY()};
	}

	Vector2 ProjectileAbility::getCasterCenter() const {
		if (!_caster)
			return {0.0f, 0.0f};

		return _caster->getCenter();
	}

} // namespace Nawia::Entity
