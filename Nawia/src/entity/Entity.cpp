#include "Entity.h"
#include <json.hpp>
#include <fstream>
#include <raymath.h>
#include "Ability.h"
#include "Collider.h"

#include <Logger.h>
#include <Map.h>
#include <Constants.h>


namespace Nawia::Entity {

	bool Entity::DebugColliders = true; // enable debug hitbox drawing

	Entity::Entity(const std::string& name, const float start_x, const float start_y, const std::shared_ptr<Texture2D>& texture, const int max_hp)
		: _name(name), _texture(texture), _max_hp(max_hp), _hp(max_hp),
		  _current_anim_index(0), _anim_frame_counter(0), _rotation(0.0f), _model_loaded(false), _use_3d_rendering(false),
		  _velocity{0.0f, 0.0f}, _scale(1.0f), _faction(Faction::None), _pos{start_x, start_y},
		  _anim_looping(true), _anim_locked(false), _hovered(false) {}

	Entity::~Entity()
	{
		if (_model_loaded)
		{
			for (const auto& anim : _animations)
				UnloadModelAnimation(anim);

			UnloadModel(_model);
			UnloadRenderTexture(_target);
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
		_use_3d_rendering = true;

		// initialize 3D Rendering
		_target = LoadRenderTexture(Core::MODEL_RENDER_SIZE, Core::MODEL_RENDER_SIZE);
		_camera.position = Core::ISOMETRIC_CAMERA_POS;
		_camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
		_camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
		_camera.fovy = 45.0f;
		_camera.projection = CAMERA_PERSPECTIVE;

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
				_anim_frame_counter = start_frame;
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
		// todo update when closer to player
		if (_model_loaded && !_animations.empty())
		{
			_anim_frame_counter++;
			UpdateModelAnimation(_model, _animations[_current_anim_index], _anim_frame_counter);

			if (_anim_frame_counter >= _animations[_current_anim_index].frameCount)
			{
				_anim_frame_counter = 0;
				if (!_anim_looping) {
					playAnimation("default", true, false);
				}
			}
		}
	}

	void Entity::render(const float offset_x, const float offset_y) 
	{
		Vector2 pos = getScreenPos(getX(), getY(), offset_x, offset_y);

		if (_use_3d_rendering && _model_loaded)
		{
			BeginTextureMode(_target);
			ClearBackground(BLANK);
			BeginMode3D(_camera);

			DrawModelEx(_model, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, _rotation, { _scale, _scale, _scale }, WHITE);
			
			EndMode3D();
			EndTextureMode();
			
			const float texture_width = static_cast<float>(_target.texture.width);
			const float texture_height = static_cast<float>(_target.texture.height);

			const Rectangle source = { 0.0f, 0.0f, texture_width, -texture_height };
			const Rectangle dest = { pos.x, pos.y, texture_width, texture_height };
			const Vector2 origin = { dest.width / 2.0f, dest.height / 2.0f };

			DrawTexturePro(_target.texture, source, dest, origin, 0.0f, WHITE);
		}
		else if (_texture)
		{
			const float source_texture_width = static_cast<float>(_texture->width);
			const float source_texture_height = static_cast<float>(_texture->height);
			constexpr float dest_texture_width = static_cast<float>(Core::ENTITY_TEXTURE_WIDTH);
			constexpr float dest_texture_height = static_cast<float>(Core::ENTITY_TEXTURE_HEIGHT);

			const Rectangle source = {0.0f, 0.0f, source_texture_width, source_texture_height };
			const Rectangle dest = {pos.x, pos.y, dest_texture_width, dest_texture_height };
			constexpr Vector2 origin = {0.0f, 0.0f};
			DrawTexturePro(*_texture, source, dest, origin, 0.0f, WHITE);
		}

		if (_hovered)
		{
			if (_use_3d_rendering && _model_loaded)
			{
			    const float texture_width = static_cast<float>(_target.texture.width);
			    const float texture_height = static_cast<float>(_target.texture.height);
			    const Rectangle source = { 0.0f, 0.0f, texture_width, -texture_height };
			    const Rectangle dest = { pos.x, pos.y, texture_width, texture_height };
			    const Vector2 origin = { dest.width / 2.0f, dest.height / 2.0f };
			    
			    DrawTexturePro(_target.texture, source, dest, origin, 0.0f, Fade(BLACK, 0.2f));
			}
			else if (_texture)
			{
			    const float source_texture_width = static_cast<float>(_texture->width);
			    const float source_texture_height = static_cast<float>(_texture->height);
			    constexpr float dest_texture_width = static_cast<float>(Core::ENTITY_TEXTURE_WIDTH);
			    constexpr float dest_texture_height = static_cast<float>(Core::ENTITY_TEXTURE_HEIGHT);

			    const Rectangle source = {0.0f, 0.0f, source_texture_width, source_texture_height };
			    const Rectangle dest = {pos.x, pos.y, dest_texture_width, dest_texture_height };
			    constexpr Vector2 origin = {0.0f, 0.0f};
				DrawTexturePro(*_texture, source, dest, origin, 0.0f, Fade(BLACK, 0.2f));
			}
		}

		if (DebugColliders && _collider) {
			_collider->render(offset_x, offset_y);
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

	bool Entity::isMouseOver(const float mouse_x, const float mouse_y, const float cam_x, const float cam_y) const 
	{
		if (_collider) {
			return _collider->checkPoint(mouse_x, mouse_y, cam_x, cam_y);
		}

		Vector2 screen_pos = getScreenPos(getX(), getY(), cam_x, cam_y);

		return (mouse_x >= screen_pos.x &&  mouse_x <= screen_pos.x + Core::ENTITY_TEXTURE_WIDTH &&
		      mouse_y >= screen_pos.y && mouse_y <= screen_pos.y + Core::ENTITY_TEXTURE_HEIGHT);
	}

	Vector2 Entity::getScreenPos(const float world_x, const float world_y, const float cam_x, const float cam_y) const
	{
		Vector2 screen_pos = getIsoPos(world_x, world_y, cam_x, cam_y);
		screen_pos.x -= Core::ENTITY_TEXTURE_WIDTH / 2.0f;
		screen_pos.y -= Core::ENTITY_TEXTURE_HEIGHT; // pivot at bottom (feet)

		return screen_pos;
	}

	Vector2 Entity::getIsoPos(const float world_x, const float world_y, const float cam_x, const float cam_y) const
	{
		float screen_x = (world_x - world_y) * (Core::TILE_WIDTH / 2.0f) + cam_x;
		float screen_y = (world_x + world_y) * (Core::TILE_HEIGHT / 2.0f) + cam_y;
		return { screen_x, screen_y };
	}

	Vector2 Entity::getCenter() const {
		if (_collider) {
			return _collider->getPosition();
		}
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
		// Standard Rotation (Feet Pivot) - Best for Movement
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
		// Combat Rotation (Center Pivot) - Best for Aiming/Interaction
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