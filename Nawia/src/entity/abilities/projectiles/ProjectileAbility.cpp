#include "ProjectileAbility.h"

#include <Entity.h>
#include <Projectile.h>
#include <ResourceManager.h>
#include <SoundIds.h>

#include <utility>

namespace Nawia::Entity {

	namespace {
		float getModelCenterHeight(const Entity& entity) {
			const BoundingBox bounding_box = entity.getBoundingBox();
			return (bounding_box.min.y + bounding_box.max.y) * 0.5f;
		}
	}

	ProjectileAbility::ProjectileAbility(std::string ability_name,
										 const std::string& stats_key,
										 const AbilityTargetType target_type,
										 std::string projectile_name,
										 std::string model_path,
										 const float model_scale,
										 const std::shared_ptr<Texture2D>& hit_texture,
										 const std::shared_ptr<Texture2D>& icon_texture,
										 const float facing_offset,
										 Core::ResourceManager* resource_manager)
		: Ability(std::move(ability_name), Entity::getAbilityStatsFromJson(stats_key), target_type, icon_texture),
		  _projectile_name(std::move(projectile_name)),
		  _model_path(std::move(model_path)),
		  _model_scale(model_scale),
		  _hit_texture(hit_texture),
		  _facing_offset(facing_offset),
		  _resource_manager(resource_manager) {}

	AbilitySpawn ProjectileAbility::cast(const float target_x, const float target_y) {
		if (!beginCast())
			return nullptr;

		const Vector2 spawn_position = getSpawnPosition();
		float target_height = _caster ? _caster->getAltitude() + 1.0f : 1.0f;
		if (_caster) {
			if (const auto target = _caster->getTarget()) {
				const Vector2 target_center = target->getCenter();
				const float dx = target_center.x - target_x;
				const float dy = target_center.y - target_y;
				if (dx * dx + dy * dy < 1.0f)
					target_height = getModelCenterHeight(*target);
			}
		}

		if (_caster) {
			if (_name == "Fireball")
				_caster->playSoundEffect(Audio::SoundId::FireballCast, 0.85f);
			else if (_name == "Knife Throw")
				_caster->playSoundEffect(Audio::SoundId::KnifeThrow, 0.8f);
		}

		const Model* shared_model = nullptr;
		if (_resource_manager && !_model_path.empty())
			shared_model = _resource_manager->getModel(_model_path);

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
			target_height,
			_hit_texture,
			_facing_offset,
			shared_model);
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
