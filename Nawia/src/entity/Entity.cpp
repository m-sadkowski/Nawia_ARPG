#include "Entity.h"
#include "Ability.h"
#include "Collider.h"

#include <Logger.h>
#include <MathUtils.h>

#include <json.hpp>
#include <fstream>
#include <raymath.h>

namespace Nawia::Entity {

	bool Entity::DebugColliders = true; // enable debug hitbox drawing

	Entity::Entity(const std::string& name, const float start_x, const float start_y, const std::shared_ptr<Texture2D>& texture, const int max_hp)
		: _name(name), _texture(texture), _max_hp(max_hp), _hp(max_hp),
		  _current_anim_index(0), _anim_frame_counter(0.0f), _rotation(0.0f), _model_loaded(false),
		  _velocity{0.0f, 0.0f}, _scale(1.0f), _faction(Faction::None), _pos{start_x, start_y},
		  _anim_looping(true), _anim_locked(false), _hovered(false) {}

	Entity::~Entity()
	{
		if (_model_loaded)
		{
			for (const auto& anim : _animations)
				UnloadModelAnimation(anim);

			UnloadModel(_model);
		}
	}

	void Entity::loadModel(const std::string& path, const bool rotate_model)
	{
		_model = LoadModel(path.c_str());
		if (_model.meshCount == 0)
		{
			Core::Logger::errorLog("Failed to load model: " + path);
			return;
		}

		// correction for Z-up models
		if (rotate_model)
			_model.transform = MatrixRotateX(-PI / 2.0f);

		_model_loaded = true;

		// load animations from the initial file
		addAnimation("default", path);
	}

	void Entity::addAnimation(const std::string& name, const std::string& path)
	{
		if (!_model_loaded)
			return;

		int count = 0;
		ModelAnimation* anims = LoadModelAnimations(path.c_str(), &count);

		if (count > 0)
		{
			const int start_index = static_cast<int>(_animations.size());

			for (int i = 0; i < count; i++)
				_animations.push_back(anims[i]);

			_animation_map[name] = start_index;

			MemFree(anims);
		}
	}

	void Entity::playAnimation(const std::string& name, const bool loop, const bool lock_movement, const int start_frame, const bool force)
	{
		if (_animation_map.find(name) != _animation_map.end())
		{
			const int index = _animation_map[name];
			if (force || index != _current_anim_index)
			{
				_current_anim_index = index;
				_anim_frame_counter = static_cast<float>(start_frame);
				_anim_looping = loop;
				_anim_locked = lock_movement;
			}
		}
	}

	int Entity::getAnimationFrameCount(const std::string& name) const
	{
		if (_animation_map.find(name) != _animation_map.end()) {
			const int index = _animation_map.at(name);
			if (index >= 0 && index < _animations.size()) {
				return _animations[index].frameCount;
			}
		}
		return 0;
	}

	void Entity::update(const float delta_time)
	{
		_pos.x += _velocity.x * delta_time;
		_pos.y += _velocity.y * delta_time;
		
		updateAnimation(delta_time);
	}

	void Entity::updateAnimation(const float dt)
	{
		if (_model_loaded && !_animations.empty())
		{
			_anim_frame_counter += dt * _anim_fps * _anim_speed_multiplier;

			if (_anim_frame_counter >= _animations[_current_anim_index].frameCount)
			{
				if (_anim_looping) {
					while (_anim_frame_counter >= _animations[_current_anim_index].frameCount) {
						_anim_frame_counter -= _animations[_current_anim_index].frameCount;
					}
				}
				else {
					_anim_frame_counter = 0;
					playAnimation("default", true, false);
				}
			}

			UpdateModelAnimation(_model, _animations[_current_anim_index], static_cast<int>(_anim_frame_counter));
		}
	}

	void Entity::render(const Camera3D& camera) 
	{
		if (_model_loaded)
		{
			const Vector3 pos3d = getWorldPos3D();
			const float visual_rotation = _rotation + _model_facing_offset;
			DrawModelEx(_model, pos3d, { 0.0f, 1.0f, 0.0f }, visual_rotation, { _scale, _scale, _scale }, WHITE);

			if (_hovered)
			{
				// Draw the model again with a dark tint overlay for hover effect
				DrawModelEx(_model, pos3d, { 0.0f, 1.0f, 0.0f }, visual_rotation, { _scale, _scale, _scale }, Fade(BLACK, 0.2f));
			}
		}

		if (DebugColliders) {
			if (_collider)
				_collider->render(camera);

			// Draw 3D bounding box (green = clickable area)
			if (_model_loaded)
			{
				const BoundingBox bbox = getBoundingBox();
				DrawBoundingBox(bbox, GREEN);
			}
		}
	}

	void Entity::takeDamage(const int dmg) 
	{
		Core::Logger::debugLog("Entity " + getName() + " taking damage: " + std::to_string(dmg) + ". Current HP: " + std::to_string(_hp));
		_hp -= dmg;
		if (_hp < 0) 
		{
			_hp = 0;
			Core::Logger::debugLog("Entity died!");
		}
	}

	void Entity::die()
	{
		_hp = 0;
		Core::Logger::debugLog("Entity " + getName() + " killed!");
	}

	bool Entity::isMouseOver(const float screen_x, const float screen_y, const Camera3D& camera) const 
	{
		if (_model_loaded)
		{
			const Ray mouse_ray = GetScreenToWorldRay(Vector2{ screen_x, screen_y }, camera);
			
			// Build the same transform matrix used by DrawModelEx:
			// translate * rotate(Y-axis) * scale * model.transform
			const Vector3 pos3d = getWorldPos3D();
			const Matrix mat_translate = MatrixTranslate(pos3d.x, pos3d.y, pos3d.z);
			const float visual_rotation = (_rotation + _model_facing_offset) * DEG2RAD;
			const Matrix mat_rotate = MatrixRotate({ 0.0f, 1.0f, 0.0f }, visual_rotation);
			const Matrix mat_scale = MatrixScale(_scale, _scale, _scale);
			const Matrix world_transform = MatrixMultiply(
				MatrixMultiply(MatrixMultiply(mat_scale, _model.transform), mat_rotate),
				mat_translate
			);

			// Test ray against each mesh's actual triangles
			for (int i = 0; i < _model.meshCount; i++)
			{
				const RayCollision collision = GetRayCollisionMesh(mouse_ray, _model.meshes[i], world_transform);
				if (collision.hit)
					return true;
			}
			return false;
		}

		// Fallback for entities without a 3D model
		const Vector2 screen_pos = getScreenPosition(camera);
		constexpr float click_radius = 30.0f;
		const float dx = screen_x - screen_pos.x;
		const float dy = screen_y - screen_pos.y;
		return (dx * dx + dy * dy) < (click_radius * click_radius);
	}

	BoundingBox Entity::getBoundingBox() const
	{
		if (!_model_loaded)
		{
			// Return a small default box at position
			const Vector3 pos = getWorldPos3D();
			return BoundingBox{
				Vector3{ pos.x - 0.5f, pos.y, pos.z - 0.5f },
				Vector3{ pos.x + 0.5f, pos.y + 1.0f, pos.z + 0.5f }
			};
		}

		// Get model-local bounding box
		const BoundingBox local_bb = GetModelBoundingBox(_model);
		const Vector3 pos = getWorldPos3D();

		// Scale and translate the bounding box to world space
		return BoundingBox{
			Vector3{
				local_bb.min.x * _scale + pos.x,
				local_bb.min.y * _scale + pos.y,
				local_bb.min.z * _scale + pos.z
			},
			Vector3{
				local_bb.max.x * _scale + pos.x,
				local_bb.max.y * _scale + pos.y,
				local_bb.max.z * _scale + pos.z
			}
		};
	}

	Vector2 Entity::getScreenPosition(const Camera3D& camera) const
	{
		const Vector3 world_pos = { _pos.x, 0.0f, _pos.y };
		return GetWorldToScreen(world_pos, camera);
	}

	Vector2 Entity::getCenter() const {
		return _pos;
	}

	AbilityStats Entity::getAbilityStatsFromJson(const std::string& name)
	{
		std::string path = "../assets/data/abilities.json";
		std::ifstream file(path);
		
		if (!file.is_open()) {
			file.open(path);
			if (!file.is_open()) {
				Core::Logger::errorLog("Entity - Couldn't open file " + path);
				return {};
			}
		}

		nlohmann::json data;
		try {
			file >> data;
		}
		catch (const nlohmann::json::parse_error& e) {
			(void)e;
			Core::Logger::errorLog("Entity - Couldn't parse json file: " + path);
			return {};
		}

		if (data.contains("abilities"))
		{
			for (const auto& ability : data["abilities"])
			{
				if (ability["name"] == name)
				{
					AbilityStats stats;
					if (ability.contains("stats")) 
					{
						const auto& json_stats = ability["stats"];
						if (json_stats.contains("damage")) stats.damage = json_stats["damage"].get<int>();
						if (json_stats.contains("cooldown")) stats.cooldown = json_stats["cooldown"].get<float>();
						if (json_stats.contains("cast_range")) stats.cast_range = json_stats["cast_range"].get<float>();
						if (json_stats.contains("projectile_speed")) stats.projectile_speed = json_stats["projectile_speed"].get<float>();
						if (json_stats.contains("duration")) stats.duration = json_stats["duration"].get<float>();
						if (json_stats.contains("hitbox_radius")) stats.hitbox_radius = json_stats["hitbox_radius"].get<float>();
					}
					return stats;
				}
			}
		}
		
		Core::Logger::errorLog("Entity - Ability not found: " + name);
		return {};
	}

	void Entity::addAbility(const std::shared_ptr<Ability>& ability) 
	{
		ability->setCaster(this);
		_abilities.push_back(ability);
	}

	std::shared_ptr<Ability> Entity::getAbility(const int index)
	{
		if (index >= 0 && index < _abilities.size())
			return _abilities[index];

		return nullptr;
	}

	void Entity::updateAbilities(const float dt) const 
	{
		for (auto &s : _abilities)
			s->update(dt);
	}

	void Entity::setCollider(std::unique_ptr<Collider> collider)
	{
		_collider = std::move(collider);
	}

	void Entity::rotateTowards(const float world_x, const float world_y)
	{
		const float dx = world_x - getX();
		const float dy = world_y - getY();

		if (dx * dx + dy * dy > 0.001f)
		{
			const float angle = std::atan2(dy, dx) * 180.0f / PI;
			setRotation(-angle);
		}
	}

	void Entity::rotateTowardsCenter(const float world_x, const float world_y)
	{
		Vector2 center = getCenter();
		const float dx = world_x - center.x;
		const float dy = world_y - center.y;

		if (dx * dx + dy * dy > 0.001f)
		{
			const float angle = std::atan2(dy, dx) * 180.0f / PI;
			setRotation(-angle);
		}
	}

} // namespace Nawia::Entity