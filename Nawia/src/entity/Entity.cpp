#include "Entity.h"
#include <Ability.h>
#include <AudioManager.h>
#include <Collider.h>
#include <ResourceManager.h>

#include <Logger.h>
#include <MathUtils.h>

#include <json.hpp>
#include <raymath.h>

#include <cmath>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <limits>
#include <map>
#include <memory>

namespace {
	constexpr const char* ABILITIES_PATH = "assets/data/abilities.json";

	nlohmann::json loadAbilitiesData()
	{
		std::ifstream file(ABILITIES_PATH);
		if (!file.is_open())
		{
			Nawia::Core::Logger::errorLog(std::string("Entity - nie można otworzyć pliku: ") + ABILITIES_PATH);
			return {};
		}

		nlohmann::json data;
		try
		{
			file >> data;
		}
		catch (const nlohmann::json::parse_error&)
		{
			Nawia::Core::Logger::errorLog(std::string("Entity - nie można sparsować JSON: ") + ABILITIES_PATH);
			return {};
		}

		return data;
	}

	template <typename T>
	void assignStatIfPresent(const nlohmann::json& json_stats, const char* key, T& destination)
	{
		if (const auto stat_it = json_stats.find(key); stat_it != json_stats.end())
			destination = stat_it->get<T>();
	}
}

namespace Nawia::Entity {

namespace {
	Core::ResourceManager* g_shared_resource_manager = nullptr;
	std::map<std::string, std::shared_ptr<const AnimationBundle>> g_animation_cache;

	std::shared_ptr<const AnimationBundle> getCachedAnimationBundle(const std::string& path)
	{
		const auto cached_animation = g_animation_cache.find(path);
		if (cached_animation != g_animation_cache.end())
			return cached_animation->second;

		int count = 0;
		ModelAnimation* anims = LoadModelAnimations(path.c_str(), &count);

		auto bundle = std::make_shared<AnimationBundle>();
		if (count > 0 && anims != nullptr)
		{
			bundle->clips.reserve(count);
			for (int i = 0; i < count; i++)
				bundle->clips.push_back(anims[i]);
		}

		if (anims != nullptr)
			MemFree(anims);

		g_animation_cache[path] = bundle;
		return bundle;
	}

	const ModelAnimation* resolveAnimation(const AnimationClipRef& animation)
	{
		if (!animation.bundle)
			return nullptr;

		if (animation.clip_index < 0 || static_cast<size_t>(animation.clip_index) >= animation.bundle->clips.size())
			return nullptr;

		return &animation.bundle->clips[animation.clip_index];
	}
}

	AnimationBundle::~AnimationBundle()
	{
		for (const auto& anim : clips)
			UnloadModelAnimation(anim);
	}

bool Entity::DebugColliders = false; // Wlaczac tylko diagnostycznie, bo render hitboxow jest drogi.
	
	Entity::Entity() 
		: _pos{0.0f, 0.0f}, _velocity{0.0f, 0.0f}, _scale(1.0f), 
		  _hp(1), _max_hp(1), _type(EntityType::None), _faction(Faction::None) {}

	Entity::Entity(const std::string& name, const float start_x, const float start_y, const std::shared_ptr<Texture2D>& texture, const int max_hp)
		: _name(name), _texture(texture), _max_hp(max_hp), _hp(max_hp),
		  _current_anim_index(0), _anim_frame_counter(0.0f), _rotation(0.0f), _model_loaded(false),
		  _velocity{0.0f, 0.0f}, _scale(1.0f), _faction(Faction::None), _pos{start_x, start_y},
		  _anim_looping(true), _anim_locked(false), _hovered(false) {}

	Entity::~Entity()
	{
		if (!_movement_sound_id.empty())
			stopSoundEffect(_movement_sound_id);

		unloadModelData();
	}

	void Entity::unloadModelData()
	{
		_animations.clear();
		_animation_map.clear();
		_animation_path_map.clear();

		if (_model_loaded && _owns_model) {
			if (_cloned_model)
				Core::ResourceManager::unloadClonedModel(_model);
			else
				UnloadModel(_model);
		}

		_model = {};
		_model_loaded = false;
		_owns_model = false;
		_cloned_model = false;
		_local_model_bounding_box = {};
		_local_model_bounding_box_valid = false;
		_current_anim_index = 0;
		_anim_frame_counter = 0.0f;
		_last_applied_anim_index = -1;
		_last_applied_anim_frame = -1;
		_anim_looping = true;
		_anim_locked = false;
		_anim_ping_pong = false;
		_anim_reverse_phase = false;
		_anim_direction = 1.0f;
	}

	void Entity::setSharedResourceManager(Core::ResourceManager* manager) {
		g_shared_resource_manager = manager;
	}

	Core::ResourceManager* Entity::getSharedResourceManager() {
		return g_shared_resource_manager;
	}

	void Entity::loadModel(const std::string& path, const bool rotate_model)
	{
		unloadModelData();

		// Klonowanie modelu z cache'u ResourceManagera: kopiujemy bufory mesh
		// w pamieci RAM i uploadujemy do nowych VBO na GPU. Jest to ~100x
		// szybsze niz LoadModel z dysku, a kazda encja dostaje wlasne bufory,
		// wiec UpdateModelAnimation nie powoduje migotania.
		if (g_shared_resource_manager) {
			Model cloned = g_shared_resource_manager->cloneModel(path);
			if (cloned.meshCount > 0) {
				if (rotate_model)
					cloned.transform = MatrixRotateX(-PI / 2.0f);

				_model = cloned;
				_model_loaded = true;
				_owns_model = true;
				_cloned_model = true;
				_local_model_bounding_box = GetModelBoundingBox(_model);
				_local_model_bounding_box_valid = true;
				_last_applied_anim_index = -1;
				_last_applied_anim_frame = -1;
				addAnimation("default", path);
				return;
			}
		}

		// Fallback: ladowanie z dysku jesli cache jest pusty.
		replaceModel(path, rotate_model);
		if (!_model_loaded)
			return;

		addAnimation("default", path);
	}

	void Entity::replaceModel(const std::string& path, const bool rotate_model)
	{
		if (_model_loaded && _owns_model) {
			if (_cloned_model)
				Core::ResourceManager::unloadClonedModel(_model);
			else
				UnloadModel(_model);
		}
		_cloned_model = false;

		_model = LoadModel(path.c_str());
		if (_model.meshCount == 0)
		{
			_model = {};
			_model_loaded = false;
			_owns_model = false;
			_local_model_bounding_box = {};
			_local_model_bounding_box_valid = false;
			Core::Logger::errorLog("Nie udało się załadować modelu: " + path);
			return;
		}

		// Korekta dla modeli zapisanych w układzie Z-up.
		if (rotate_model)
			_model.transform = MatrixRotateX(-PI / 2.0f);

		_model_loaded = true;
		_owns_model = true;
		_local_model_bounding_box = GetModelBoundingBox(_model);
		_local_model_bounding_box_valid = true;

		_last_applied_anim_index = -1;
		_last_applied_anim_frame = -1;
	}

	void Entity::useSharedModel(const Model& model)
	{
		unloadModelData();

		if (model.meshCount == 0)
			return;

		_model = model;
		_model_loaded = true;
		_owns_model = false;
		_local_model_bounding_box = GetModelBoundingBox(_model);
		_local_model_bounding_box_valid = true;
	}

	void Entity::addAnimation(const std::string& name, const std::string& path)
	{
		if (!_model_loaded)
			return;

		if (const auto cached_path = _animation_path_map.find(path); cached_path != _animation_path_map.end())
		{
			_animation_map[name] = cached_path->second;
			return;
		}

		const auto bundle = getCachedAnimationBundle(path);
		if (!bundle || bundle->clips.empty())
			return;

		const int start_index = static_cast<int>(_animations.size());

		for (int i = 0; i < static_cast<int>(bundle->clips.size()); i++)
			_animations.push_back({bundle, i});

		_animation_map[name] = start_index;
		_animation_path_map[path] = start_index;
	}

	void Entity::loadAnimationBundle(const std::string& path) {
		if (!_model_loaded)
			return;

		const auto bundle = getCachedAnimationBundle(path);
		if (!bundle || bundle->clips.empty())
			return;

		const int start_index = static_cast<int>(_animations.size());

		for (int i = 0; i < static_cast<int>(bundle->clips.size()); i++) {
			_animations.push_back({bundle, i});

			std::string anim_name = bundle->clips[i].name;

			if (anim_name.empty()) {
				anim_name = "anim_" + std::to_string(start_index + i);
			}

			_animation_map[anim_name] = start_index + i;
		}

		_animation_path_map[path] = start_index;
	}

	void Entity::preloadAnimationData(const std::string& path)
	{
		if (!path.empty())
			getCachedAnimationBundle(path);
	}

	void Entity::playAnimation(const std::string& name, const bool loop, const bool lock_movement, const int start_frame, const bool force)
	{
		const auto animation_it = _animation_map.find(name);
		if (animation_it != _animation_map.end())
		{
			const int index = animation_it->second;
			if (force || index != _current_anim_index)
			{
				_current_anim_index = index;
				_anim_frame_counter = static_cast<float>(start_frame);
				_anim_looping = loop;
				_anim_locked = lock_movement;
				_anim_ping_pong = false;
				_anim_reverse_phase = false;
				_anim_direction = 1.0f;
				_last_applied_anim_index = -1;
				_last_applied_anim_frame = -1;
			}
		}
	}

	void Entity::playAnimationPingPong(
		const std::string& name,
		const bool lock_movement,
		const int start_frame,
		const bool force)
	{
		playAnimation(name, false, lock_movement, start_frame, force);

		if (_animation_map.find(name) == _animation_map.end())
			return;

		_anim_ping_pong = true;
		_anim_reverse_phase = false;
		_anim_direction = 1.0f;
	}

	int Entity::getAnimationFrameCount(const std::string& name) const
	{
		const auto animation_it = _animation_map.find(name);
		if (animation_it == _animation_map.end())
			return 0;

		const int index = animation_it->second;
		if (index < 0 || static_cast<size_t>(index) >= _animations.size())
			return 0;

		const ModelAnimation* animation = resolveAnimation(_animations[index]);
		return animation ? animation->frameCount : 0;
	}

	void Entity::update(const float delta_time)
	{
		if (_dormant) return;

		if (_is_dying)
		{
			updateAnimation(delta_time);
			if (!isAnimationLocked())
				_hp = 0;
			return; // Podczas śmierci nie aktualizujemy ruchu ani logiki encji.
		}

		_pos.x += _velocity.x * delta_time;
		_pos.y += _velocity.y * delta_time;
		
		updateAnimation(delta_time);
	}

	void Entity::updateAnimation(const float dt)
	{
		if (_model_loaded && !_animations.empty())
		{
			if (_current_anim_index < 0 || static_cast<size_t>(_current_anim_index) >= _animations.size())
				return;

			const ModelAnimation* current_animation = resolveAnimation(_animations[_current_anim_index]);
			if (!current_animation || current_animation->frameCount <= 0)
				return;

			_anim_frame_counter += dt * _anim_fps * _anim_speed_multiplier * _anim_direction / ANIMATION_DURATION_SCALE;

			if (_anim_frame_counter >= current_animation->frameCount)
			{
				if (_anim_looping) {
					while (_anim_frame_counter >= current_animation->frameCount)
						_anim_frame_counter -= current_animation->frameCount;
				}
				else if (_anim_ping_pong && !_anim_reverse_phase) {
					_anim_reverse_phase = true;
					_anim_direction = -1.0f;
					_anim_frame_counter = static_cast<float>(current_animation->frameCount - 1);
				}
				else {
					_anim_frame_counter = 0;
					if (getAnimationFrameCount("Idle_Loop") > 0)
						playAnimation("Idle_Loop", true, false, 0, true);
					else
						playAnimation("default", true, false, 0, true);

					current_animation = resolveAnimation(_animations[_current_anim_index]);
					if (!current_animation || current_animation->frameCount <= 0)
						return;
				}
			}
			else if (_anim_frame_counter <= 0.0f && _anim_direction < 0.0f)
			{
				_anim_frame_counter = 0.0f;
				_anim_ping_pong = false;
				_anim_reverse_phase = false;
				_anim_direction = 1.0f;

				if (getAnimationFrameCount("Idle_Loop") > 0)
					playAnimation("Idle_Loop", true, false, 0, true);
				else
					playAnimation("default", true, false, 0, true);

				current_animation = resolveAnimation(_animations[_current_anim_index]);
				if (!current_animation || current_animation->frameCount <= 0)
					return;
			}

			const int animation_frame = static_cast<int>(_anim_frame_counter);
			if (_last_applied_anim_index != _current_anim_index || _last_applied_anim_frame != animation_frame)
			{
				UpdateModelAnimation(_model, *current_animation, animation_frame);
				if (_equipment)
					_equipment->updateAnimations(*current_animation, animation_frame);

				_last_applied_anim_index = _current_anim_index;
				_last_applied_anim_frame = animation_frame;
			}
		}
	}

	void Entity::render(const Camera3D& camera) 
	{
		if (_dormant) return;

		if (_model_loaded)
		{
			const Vector3 pos3d = getWorldPos3D();
			const float visual_rotation = _rotation + _model_facing_offset;
			DrawModelEx(_model, pos3d, { 0.0f, 1.0f, 0.0f }, visual_rotation, { _scale, _scale, _scale }, WHITE);
			if (_equipment) {
				_equipment->draw(pos3d, visual_rotation, _rotation, _scale);
			}

			if (_hovered)
			{
				// Drugi przebieg renderu daje subtelne przyciemnienie przy hoverze.
				DrawModelEx(_model, pos3d, { 0.0f, 1.0f, 0.0f }, visual_rotation, { _scale, _scale, _scale }, Fade(BLACK, 0.2f));
			}
		}

		if (DebugColliders)
		{
			if (_collider)
				_collider->render(camera);

			// Zielone pudełko ograniczające pokazuje obszar kliknięcia i szybki test wstępny.
			if (_model_loaded)
			{
				const BoundingBox bbox = getBoundingBox();
				DrawBoundingBox(bbox, GREEN);
			}
		}
	}

	void Entity::takeDamage(const int dmg) 
	{
		if (_is_dying) return;

		Core::Logger::debugLog("Entity " + getName() + " otrzymuje obrażenia: " + std::to_string(dmg) + ". Obecne HP: " + std::to_string(_hp));
		if (_hp - dmg <= 0) 
		{
			_hp = 1; // Utrzymujemy encję przy życiu do końca animacji śmierci.
			_is_dying = true;
			playAnimation(_death_anim_name, false, true, 0, true);
			setFaction(Faction::None);
			onDeathStarted();
			Core::Logger::debugLog("Entity " + getName() + " rozpoczęła sekwencję śmierci.");
		}
		else
		{
			_hp -= dmg;
		}
	}

	void Entity::die()
	{
		_hp = 0;
		Core::Logger::debugLog("Entity " + getName() + " została zabita.");
	}

	void Entity::setMaxHp(const int max_hp)
	{
		_max_hp = max_hp;
		_hp = max_hp;
	}

	void Entity::setHP(const int hp)
	{
		_hp = std::clamp(hp, 0, _max_hp);
		_is_dying = false;
		if (_hp > 0 && _type != EntityType::Projectile)
			setFaction(_faction);
	}

	nlohmann::json Entity::serializeState() const
	{
		return {
			{"name", _name},
			{"position", {{"x", _pos.x}, {"y", _pos.y}}},
			{"altitude", _altitude},
			{"rotation", _rotation},
			{"hp", _hp},
			{"max_hp", _max_hp},
			{"dead", isDead()},
			{"dormant", _dormant}
		};
	}

	void Entity::applyState(const nlohmann::json& state, Item::ItemDatabase* /*item_database*/)
	{
		if (!state.is_object())
			return;

		if (state.contains("position") && state["position"].is_object()) {
			_pos.x = state["position"].value("x", _pos.x);
			_pos.y = state["position"].value("y", _pos.y);
		}

		_altitude = state.value("altitude", _altitude);
		_rotation = state.value("rotation", _rotation);

		if (state.contains("max_hp") && state["max_hp"].is_number_integer())
			setMaxHp(state["max_hp"].get<int>());

		const int loaded_hp = state.value("hp", _hp);
		if (state.value("dead", false) || loaded_hp <= 0) {
			die();
			_dormant = true;
		} else {
			setHP(loaded_hp);
			_dormant = state.value("dormant", _dormant);
		}
	}

	bool Entity::isMouseOver(const float screen_x, const float screen_y, const Camera3D& camera) const 
	{
		if (_model_loaded)
		{
			const Ray mouse_ray = GetScreenToWorldRay(Vector2{ screen_x, screen_y }, camera);
			if (!GetRayCollisionBox(mouse_ray, getBoundingBox()).hit)
				return false;

			return checkRayHitsMesh(mouse_ray);
		}

		// Awaryjne sprawdzanie encji bez modelu 3D.
		const Vector2 screen_pos = getScreenPosition(camera);
		constexpr float click_radius = 30.0f;
		const float dx = screen_x - screen_pos.x;
		const float dy = screen_y - screen_pos.y;
		return (dx * dx + dy * dy) < (click_radius * click_radius);
	}

	Matrix Entity::getWorldTransformMatrix() const
	{
		const Vector3 pos3d = getWorldPos3D();
		const Matrix mat_translate = MatrixTranslate(pos3d.x, pos3d.y, pos3d.z);
		const float visual_rotation = (_rotation + _model_facing_offset) * DEG2RAD;
		const Matrix mat_rotate = MatrixRotate({ 0.0f, 1.0f, 0.0f }, visual_rotation);
		const Matrix mat_scale = MatrixScale(_scale, _scale, _scale);
		return MatrixMultiply(
			MatrixMultiply(MatrixMultiply(mat_scale, _model.transform), mat_rotate),
			mat_translate);
	}

	bool Entity::checkRayHitsMesh(const Ray& ray) const
	{
		if (!_model_loaded) return false;
		if (!GetRayCollisionBox(ray, getBoundingBox()).hit)
			return false;

		const Matrix world_transform = getWorldTransformMatrix();
		for (int i = 0; i < _model.meshCount; i++)
		{
			const RayCollision collision = GetRayCollisionMesh(ray, _model.meshes[i], world_transform);
			if (collision.hit)
				return true;
		}
		return false;
	}

	RayCollision Entity::getRayMeshCollision(const Ray& ray) const
	{
		RayCollision closest = {};
		closest.hit = false;
		closest.distance = 1e30f;

		if (!_model_loaded) return closest;
		if (!GetRayCollisionBox(ray, getBoundingBox()).hit)
			return closest;

		const Matrix world_transform = getWorldTransformMatrix();
		for (int i = 0; i < _model.meshCount; i++)
		{
			const RayCollision collision = GetRayCollisionMesh(ray, _model.meshes[i], world_transform);
			if (collision.hit && collision.distance < closest.distance)
				closest = collision;
		}
		return closest;
	}

	BoundingBox Entity::getBoundingBox() const
	{
		if (!_model_loaded)
		{
		// Małe domyślne pudełko pozwala klikać encje bez modelu.
			const Vector3 pos = getWorldPos3D();
			return BoundingBox{
				Vector3{ pos.x - 0.5f, pos.y, pos.z - 0.5f },
				Vector3{ pos.x + 0.5f, pos.y + 1.0f, pos.z + 0.5f }
			};
		}

		// Pudelko ograniczajace z rayliba jest lokalne wzgledem modelu.
		const BoundingBox local_bb = _local_model_bounding_box_valid ? _local_model_bounding_box : GetModelBoundingBox(_model);
		const Vector3 pos = getWorldPos3D();

		// Skalujemy i przesuwamy pudełko do przestrzeni świata.
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
		static const nlohmann::json data = loadAbilitiesData();

		if (data.contains("abilities"))
		{
			for (const auto& ability : data["abilities"])
			{
				if (ability.value("name", "") == name)
				{
					AbilityStats stats;
					if (const auto stats_it = ability.find("stats"); stats_it != ability.end() && stats_it->is_object())
					{
						const auto& json_stats = *stats_it;
						assignStatIfPresent(json_stats, "damage", stats.damage);
						assignStatIfPresent(json_stats, "cooldown", stats.cooldown);
						assignStatIfPresent(json_stats, "cast_range", stats.cast_range);
						assignStatIfPresent(json_stats, "projectile_speed", stats.projectile_speed);
						assignStatIfPresent(json_stats, "duration", stats.duration);
						assignStatIfPresent(json_stats, "hitbox_radius", stats.hitbox_radius);
					}
					return stats;
				}
			}
		}
		
		Core::Logger::errorLog("Entity - nie znaleziono umiejętności: " + name);
		return {};
	}

	void Entity::addAbility(const std::shared_ptr<Ability>& ability) 
	{
		if (!ability) return;

		ability->setCaster(this);
		_abilities.push_back(ability);
	}

	std::shared_ptr<Ability> Entity::getAbility(const int index)
	{
		if (index < 0)
			return nullptr;

		const auto ability_index = static_cast<size_t>(index);
		if (ability_index < _abilities.size())
			return _abilities[ability_index];

		return nullptr;
	}

	void Entity::updateAbilities(const float dt) const 
	{
		for (const auto& ability : _abilities)
			ability->update(dt);
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

	void Entity::moveTo(const float x, const float y)
	{
		_target_x = x;
		_target_y = y;

		const float dx = _target_x - getX();
		const float dy = _target_y - getY();
		
		_is_moving = dx * dx + dy * dy > 0.001f;
	}

	void Entity::updateMovement(const float dt)
	{
		if (!_is_moving) return;

		const float dx = _target_x - getX();
		const float dy = _target_y - getY();
		const float distance = std::sqrt(dx * dx + dy * dy);

		if (distance <= 0.001f)
		{
			_pos.x = _target_x;
			_pos.y = _target_y;
			_is_moving = false;
			return;
		}

		if (distance > 0.001f)
			rotateTowards(_target_x, _target_y);

		const float speed = _movement_speed * _speed_multiplier;
		const float move_dist = speed * dt;

		if (move_dist >= distance) 
		{
			_pos.x = _target_x;
			_pos.y = _target_y;
			_is_moving = false;
		} 
		else 
		{
			_pos.x += (dx / distance) * move_dist;
			_pos.y += (dy / distance) * move_dist;
		}
	}

	float Entity::getDistanceToTarget() const
	{
		const auto target = _target.lock();
		if (!target) return std::numeric_limits<float>::max();
		
		const Vector2 my_pos = getCenter();
		const Vector2 target_pos = target->getCenter();
		
		return Vector2Distance(my_pos, target_pos);
	}

	Vector2 Entity::getTargetPosition() const
	{
		const auto target = _target.lock();
		if (!target) return _pos;
		
		return target->getCenter();
	}

	bool Entity::hasValidTarget() const
	{
		const auto target = _target.lock();
		return target && !target->isDead();
	}

	void Entity::chaseTarget(const float dt, const float path_recalc_interval)
	{
		if (!hasValidTarget()) return;
		
		_path_recalc_timer -= dt;
		
		if (_path_recalc_timer <= 0.0f || !_is_moving)
		{
			const Vector2 target_pos = getTargetPosition();
			moveTo(target_pos.x, target_pos.y);
			_path_recalc_timer = path_recalc_interval;
		}
		
		updateMovement(dt);
	}

	void Entity::playSoundEffect(const std::string& id, const float volume, const bool restart_if_playing, const float pitch) const
	{
		if (!_audio_manager)
			return;

		_audio_manager->playSound(id, Audio::SoundOptions{volume, pitch, restart_if_playing});
	}

	void Entity::stopSoundEffect(const std::string& id) const
	{
		if (!_audio_manager)
			return;

		_audio_manager->stopSound(id);
	}

	void Entity::updateMovementSound(const std::string& path, const bool should_play, const float volume, const float pitch)
	{
		if (!_audio_manager)
			return;

		if (_movement_sound_id.empty())
			_movement_sound_id = "movement:" + std::to_string(reinterpret_cast<std::uintptr_t>(this));

		if (should_play)
			_audio_manager->playSoundFile(_movement_sound_id, path, Audio::SoundOptions{volume, pitch, false});
		else
			_audio_manager->stopSound(_movement_sound_id);
	}

} // namespace Nawia::Entity
