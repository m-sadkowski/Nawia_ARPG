#include "Entity.h"

#include <EntityAbilityController.h>
#include <EntityAbilityConfig.h>
#include <EntityAnimationSupport.h>
#include <EntityAudioController.h>
#include <EntityMovementSupport.h>
#include <EntityPendingSpawnQueue.h>
#include <EntityStatusController.h>
#include <EntityTargetingSupport.h>
#include <EntityVisualState.h>
#include <Ability.h>
#include <AudioManager.h>
#include <CombatEventBus.h>
#include <Collider.h>
#include <ResourceManager.h>
#include <Logger.h>
#include <MathUtils.h>

#include <json.hpp>
#include <raymath.h>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>

namespace Nawia::Entity {

namespace {
	Core::ResourceManager* g_shared_resource_manager = nullptr;
	Game::CombatEventBus* g_combat_event_bus = nullptr;
}

bool Entity::DebugColliders = false; // Wlaczac tylko diagnostycznie, bo render hitboxow jest drogi.
	
	Entity::Entity() 
		: _pos{0.0f, 0.0f}, _velocity{0.0f, 0.0f}, _scale(1.0f), 
		  _hp(1), _max_hp(1), _type(EntityType::None), _faction(Faction::None),
		  _ability_controller(std::make_unique<EntityAbilityController>()),
		  _audio_controller(std::make_unique<EntityAudioController>()),
		  _pending_spawn_queue(std::make_unique<EntityPendingSpawnQueue>()),
		  _status_controller(std::make_unique<EntityStatusController>()),
		  _visual_state(std::make_unique<EntityVisualState>()) {}

	Entity::Entity(const std::string& name, const float start_x, const float start_y, const std::shared_ptr<Texture2D>& texture, const int max_hp)
		: _name(name), _texture(texture), _max_hp(max_hp), _hp(max_hp),
		  _current_anim_index(0), _anim_frame_counter(0.0f), _rotation(0.0f), _model_loaded(false),
		  _velocity{0.0f, 0.0f}, _scale(1.0f), _faction(Faction::None), _pos{start_x, start_y},
		  _anim_looping(true), _anim_locked(false),
		  _ability_controller(std::make_unique<EntityAbilityController>()),
		  _audio_controller(std::make_unique<EntityAudioController>()),
		  _pending_spawn_queue(std::make_unique<EntityPendingSpawnQueue>()),
		  _status_controller(std::make_unique<EntityStatusController>()),
		  _visual_state(std::make_unique<EntityVisualState>()) {}

	Entity::~Entity()
	{
		_audio_controller->stopMovementSound(_audio_manager);

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

	void Entity::setCombatEventBus(Game::CombatEventBus* event_bus) {
		g_combat_event_bus = event_bus;
	}

	Game::CombatEventBus* Entity::getCombatEventBus() {
		return g_combat_event_bus;
	}

	void Entity::assignEntityId(const EntityId entity_id) {
		if (_entity_id == INVALID_ENTITY_ID && entity_id != INVALID_ENTITY_ID)
			_entity_id = entity_id;
	}

	void Entity::beginCastTelemetry(std::string cast_name, const float duration_seconds, const bool interruptible) {
		_status_controller->beginCast(std::move(cast_name), duration_seconds, interruptible);
	}

	void Entity::clearCastTelemetry() {
		_status_controller->clearCast();
	}

	const EntityCastState& Entity::getCastState() const {
		return _status_controller->castState();
	}

	bool Entity::isCasting() const {
		return _status_controller->isCasting();
	}

	DamageSourceContext Entity::makeDamageSourceContext(Entity* source, std::string source_label) {
		DamageSourceContext context;
		if (!source)
			return context;

		context.valid = true;
		context.source = source->weak_from_this();
		context.source_id = source->getEntityId();
		context.source_name = source->getName();
		context.source_type = source->getType();
		context.source_faction = source->getFaction();
		context.source_position = source->getCenter();
		context.label = std::move(source_label);
		return context;
	}

	void Entity::setAudioListener(const std::shared_ptr<Entity>& listener) {
		EntityAudioController::setListener(listener);
	}

	void Entity::hideMeshIndex(const int mesh_index) {
		_visual_state->hideMeshIndex(mesh_index);
	}

	void Entity::setModelTint(const Color tint) {
		_visual_state->setModelTint(tint);
	}

	Color Entity::getModelTint() const {
		return _visual_state->modelTint();
	}

	void Entity::setHovered(const bool hovered) {
		_visual_state->setHovered(hovered);
	}

	void Entity::setModelFacingOffset(const float deg) {
		_visual_state->setModelFacingOffset(deg);
	}

	float Entity::getModelFacingOffset() const {
		return _visual_state->modelFacingOffset();
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
				ensureMeshAnimationBuffers(_model);
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

		ensureMeshAnimationBuffers(_model);

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

	bool Entity::alignLoadedModelToGround()
	{
		if (!_model_loaded)
			return false;

		const BoundingBox bounds = _local_model_bounding_box_valid
			? _local_model_bounding_box
			: GetModelBoundingBox(_model);
		_model.transform = MatrixMultiply(MatrixTranslate(0.0f, -bounds.min.y, 0.0f), _model.transform);
		return true;
	}

	void Entity::renderLoadedModel(const Color tint) const
	{
		if (!_model_loaded)
			return;

		DrawModelEx(
			_model,
			getWorldPos3D(),
			{0.0f, 1.0f, 0.0f},
			getRotation(),
			{getScale(), getScale(), getScale()},
			tint);
	}

	bool Entity::fitLoadedModelToHeight(const float target_height, const bool center_xz, const bool align_to_ground)
	{
		if (!_model_loaded || target_height <= 0.0f)
			return false;

		_local_model_bounding_box = GetModelBoundingBox(_model);
		_local_model_bounding_box_valid = true;
		const BoundingBox bounds = _local_model_bounding_box;
		const float model_height = bounds.max.y - bounds.min.y;
		if (model_height <= 1e-8f)
			return false;

		setScale(target_height / model_height);

		const float offset_x = center_xz ? -0.5f * (bounds.min.x + bounds.max.x) : 0.0f;
		const float offset_y = align_to_ground ? -bounds.min.y : 0.0f;
		const float offset_z = center_xz ? -0.5f * (bounds.min.z + bounds.max.z) : 0.0f;
		_model.transform = MatrixMultiply(MatrixTranslate(offset_x, offset_y, offset_z), _model.transform);
		return true;
	}

	void Entity::addAnimation(const std::string& name, const std::string& path)
	{
		addAnimation(name, path, 0);
	}

	void Entity::addAnimation(const std::string& name, const std::string& path, const int clip_index)
	{
		if (!_model_loaded)
			return;

		if (const auto cached_path = _animation_path_map.find(path); cached_path != _animation_path_map.end())
		{
			const int animation_index = cached_path->second + clip_index;
			if (clip_index >= 0 && static_cast<size_t>(animation_index) < _animations.size())
				_animation_map[name] = animation_index;
			return;
		}

		const auto bundle = getCachedAnimationBundle(path);
		if (!bundle || bundle->clips.empty() || clip_index < 0 || static_cast<size_t>(clip_index) >= bundle->clips.size())
			return;

		const int start_index = static_cast<int>(_animations.size());

		for (int i = 0; i < static_cast<int>(bundle->clips.size()); i++)
			_animations.push_back({bundle, i});

		_animation_map[name] = start_index + clip_index;
		_animation_path_map[path] = start_index;
	}

	void Entity::loadAnimationBundle(const std::string& path) {
		if (!_model_loaded)
			return;

		const auto bundle = getCachedAnimationBundle(path);
		if (!bundle || bundle->clips.empty())
			return;

		if (const auto cached_path = _animation_path_map.find(path); cached_path != _animation_path_map.end()) {
			const int start_index = cached_path->second;
			for (int i = 0; i < static_cast<int>(bundle->clips.size()); i++) {
				std::string anim_name = bundle->clips[i].name;
				if (anim_name.empty())
					anim_name = "anim_" + std::to_string(start_index + i);

				_animation_map[anim_name] = start_index + i;
			}
			return;
		}

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
			(void)getCachedAnimationBundle(path);
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
				_freeze_animation_on_completion = false;
				_animation_frozen_at_last_frame = false;
				_anim_direction = 1.0f;
				_last_applied_anim_index = -1;
				_last_applied_anim_frame = -1;
				applyCurrentAnimationFrame();
			}
		}
	}

	void Entity::playAnimationFreezeOnLastFrame(
		const std::string& name,
		const bool lock_movement,
		const int start_frame,
		const bool force)
	{
		playAnimation(name, false, lock_movement, start_frame, force);
		if (_animation_map.find(name) != _animation_map.end())
			_freeze_animation_on_completion = true;
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

	bool Entity::hasAnimationReachedFrame(const float frame) const
	{
		return _anim_frame_counter >= frame;
	}

	void Entity::holdAnimationFrame(const std::string& animation_name, const int frame)
	{
		setAnimationSpeed(0.0f);
		playAnimation(animation_name, true, false, frame, true);
		applyCurrentAnimationFrame();
	}

	void Entity::playAnimationReverseOnce(const std::string& animation_name, const bool lock_movement)
	{
		const int frame_count = getAnimationFrameCount(animation_name);
		if (frame_count <= 0)
			return;

		setAnimationSpeed(1.0f);
		playAnimation(animation_name, false, lock_movement, frame_count - 1, true);
		_anim_direction = -1.0f;
		_anim_looping = false;
		_anim_locked = lock_movement;
		_anim_ping_pong = false;
		_anim_reverse_phase = false;
		_last_applied_anim_index = -1;
		_last_applied_anim_frame = -1;
		applyCurrentAnimationFrame();
	}

	bool Entity::advanceAnimationTowardFrame(const float dt, const int target_frame, const bool lock_when_reached)
	{
		const float final_frame = static_cast<float>(std::max(0, target_frame));
		_anim_frame_counter += dt * _anim_fps * getAnimationSpeed() / ANIMATION_DURATION_SCALE;
		if (_anim_frame_counter >= final_frame) {
			_anim_frame_counter = final_frame;
			_anim_locked = lock_when_reached;
			applyCurrentAnimationFrame();
			return true;
		}

		applyCurrentAnimationFrame();
		return false;
	}

	void Entity::applyCurrentAnimationFrame()
	{
		if (!_model_loaded || _animations.empty())
			return;

		if (_current_anim_index < 0 || static_cast<size_t>(_current_anim_index) >= _animations.size())
			return;

		const ModelAnimation* current_animation = resolveAnimation(_animations[_current_anim_index]);
		if (!current_animation || current_animation->frameCount <= 0)
			return;

		// Walidacja: kazdy mesh musi miec boneWeights i boneIds, inaczej
		// UpdateModelAnimation uderzy w nullptr (np. female_warrior.glb).
		for (int i = 0; i < _model.meshCount; i++) {
			if (_model.meshes[i].boneWeights == nullptr || _model.meshes[i].boneIds == nullptr)
				return;
		}

		const int animation_frame = std::clamp(
			static_cast<int>(_anim_frame_counter),
			0,
			current_animation->frameCount - 1
		);
		if (_last_applied_anim_index == _current_anim_index && _last_applied_anim_frame == animation_frame)
			return;

		UpdateModelAnimation(_model, *current_animation, animation_frame);
		updateAttachedModelAnimation(*current_animation, animation_frame);

		_last_applied_anim_index = _current_anim_index;
		_last_applied_anim_frame = animation_frame;
	}

	void Entity::update(const float delta_time)
	{
		if (_dormant) return;

		updateCastTelemetry(delta_time);
		updateStatusEffects(delta_time);

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

	void Entity::updateCastTelemetry(const float dt) {
		_status_controller->updateCast(dt);
	}

	void Entity::updateStatusEffects(const float dt)
	{
		if (_status_controller->updateRoot(dt)) {
			_velocity = {0.0f, 0.0f};
			_is_moving = false;
		}

		for (const auto& tick : _status_controller->updatePoison(dt, !isDead() && !isDying())) {
			if (isDead() || isDying())
				break;
			takeDamage(tick.damage, tick.source);
		}
	}

	void Entity::applyRoot(const float duration)
	{
		_status_controller->applyRoot(duration);
		_velocity = {0.0f, 0.0f};
		_is_moving = false;
	}

	void Entity::applyPoison(const float duration, const int damage_per_tick, const float tick_interval)
	{
		applyPoison(duration, damage_per_tick, tick_interval, _last_damage_source);
	}

	void Entity::applyPoison(
		const float duration,
		const int damage_per_tick,
		const float tick_interval,
		const DamageSourceContext& source_context)
	{
		_status_controller->applyPoison(duration, damage_per_tick, tick_interval, source_context);
	}

	void Entity::clearStatusEffects()
	{
		_status_controller->clearStatusEffects();
	}

	bool Entity::isMovementRooted() const
	{
		return _status_controller->isMovementRooted();
	}

	bool Entity::isPoisoned() const
	{
		return _status_controller->isPoisoned();
	}

	float Entity::getRootRemaining() const
	{
		return _status_controller->rootRemaining();
	}

	float Entity::getPoisonRemaining() const
	{
		return _status_controller->poisonRemaining();
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

			if (_animation_frozen_at_last_frame)
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
				else if (_freeze_animation_on_completion) {
					_freeze_animation_on_completion = false;
					_anim_frame_counter = static_cast<float>(current_animation->frameCount - 1);
					_anim_locked = false;
					_animation_frozen_at_last_frame = true;
				}
				else {
					_anim_frame_counter = 0;
					if (getAnimationFrameCount("Idle_Loop") > 0)
						playAnimation("Idle_Loop", true, false, 0, true);
					else if (getAnimationFrameCount("Idle") > 0)
						playAnimation("Idle", true, false, 0, true);
					else if (getAnimationFrameCount("idle") > 0)
						playAnimation("idle", true, false, 0, true);
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
				else if (getAnimationFrameCount("Idle") > 0)
					playAnimation("Idle", true, false, 0, true);
				else if (getAnimationFrameCount("idle") > 0)
					playAnimation("idle", true, false, 0, true);
				else
					playAnimation("default", true, false, 0, true);

				current_animation = resolveAnimation(_animations[_current_anim_index]);
				if (!current_animation || current_animation->frameCount <= 0)
					return;
			}

			applyCurrentAnimationFrame();
		}
	}

	void Entity::render(const Camera3D& camera) 
	{
		if (_dormant) return;

		if (_model_loaded)
		{
			const Vector3 pos3d = getWorldPos3D();
			const float visual_rotation = _rotation + _visual_state->modelFacingOffset();
			auto draw_model = [this, pos3d, visual_rotation](const Color tint) {
				if (!_visual_state->hasHiddenMeshes()) {
					DrawModelEx(_model, pos3d, { 0.0f, 1.0f, 0.0f }, visual_rotation, { _scale, _scale, _scale }, tint);
					return;
				}

				Model model = _model;
				const Matrix mat_scale = MatrixScale(_scale, _scale, _scale);
				const Matrix mat_rotation = MatrixRotate({0.0f, 1.0f, 0.0f}, visual_rotation * DEG2RAD);
				const Matrix mat_translation = MatrixTranslate(pos3d.x, pos3d.y, pos3d.z);
				model.transform = MatrixMultiply(model.transform, MatrixMultiply(MatrixMultiply(mat_scale, mat_rotation), mat_translation));

				for (int mesh_index = 0; mesh_index < model.meshCount; ++mesh_index) {
					if (_visual_state->isMeshHidden(mesh_index))
						continue;

					const int material_index = model.meshMaterial ? model.meshMaterial[mesh_index] : 0;
					Color original = model.materials[material_index].maps[MATERIAL_MAP_DIFFUSE].color;
					Color color_tint = {
						static_cast<unsigned char>((static_cast<int>(original.r) * static_cast<int>(tint.r)) / 255),
						static_cast<unsigned char>((static_cast<int>(original.g) * static_cast<int>(tint.g)) / 255),
						static_cast<unsigned char>((static_cast<int>(original.b) * static_cast<int>(tint.b)) / 255),
						static_cast<unsigned char>((static_cast<int>(original.a) * static_cast<int>(tint.a)) / 255)
					};
					model.materials[material_index].maps[MATERIAL_MAP_DIFFUSE].color = color_tint;
					DrawMesh(model.meshes[mesh_index], model.materials[material_index], model.transform);
					model.materials[material_index].maps[MATERIAL_MAP_DIFFUSE].color = original;
				}
			};

			draw_model(_visual_state->modelTint());
			drawAttachedModel(pos3d, visual_rotation);

			if (_visual_state->hovered() && _type != EntityType::Player)
			{
				// Drugi przebieg renderu daje subtelne przyciemnienie przy hoverze.
				draw_model(Fade(BLACK, 0.2f));
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

	void Entity::takeDamage(const int dmg, const DamageSourceContext& source_context)
	{
		_last_damage_source = source_context;
		takeDamage(dmg);
	}

	void Entity::takeDamage(const int dmg) 
	{
		if (_is_dying) return;

		Core::Logger::debugLog("Entity " + getName() + " otrzymuje obrażenia: " + std::to_string(dmg) + ". Obecne HP: " + std::to_string(_hp));
		const int hp_before = _hp;
		const int hp_after = std::clamp(_hp - dmg, 0, _max_hp);
		const DamageSourceContext damage_source = _last_damage_source;

		if (g_combat_event_bus && dmg > 0) {
			g_combat_event_bus->emitDamageDealt(
				damage_source,
				this,
				dmg,
				hp_before,
				hp_after,
				damage_source.label);
		}

		if (_hp - dmg <= 0) 
		{
			const bool killed_player_side = _type == EntityType::Player || _type == EntityType::Ally;
			_hp = 1; // Utrzymujemy encję przy życiu do końca animacji śmierci.
			_is_dying = true;
			clearCastTelemetry();
			playAnimation(_death_anim_name, false, true, 0, true);
			setFaction(Faction::None);
			onDeathStarted();
			if (!_combat_death_event_emitted && g_combat_event_bus) {
				g_combat_event_bus->emitEntityKilled(damage_source, this, damage_source.label);
				_combat_death_event_emitted = true;
			}
			const auto live_damage_source = damage_source.source.lock();
			if (killed_player_side && live_damage_source && live_damage_source->healsToFullOnKill() && !live_damage_source->isDead() && !live_damage_source->isDying())
				live_damage_source->setHP(live_damage_source->getMaxHP());
			Core::Logger::debugLog("Entity " + getName() + " rozpoczęła sekwencję śmierci.");
		}
		else
		{
			_hp -= dmg;
		}

		_last_damage_source.label.clear();
	}

	void Entity::die()
	{
		const DamageSourceContext damage_source = _last_damage_source;
		_hp = 0;
		clearCastTelemetry();
		if (!_combat_death_event_emitted && g_combat_event_bus) {
			g_combat_event_bus->emitEntityKilled(damage_source, this, damage_source.label);
			_combat_death_event_emitted = true;
		}
		_last_damage_source.label.clear();
		Core::Logger::debugLog("Entity " + getName() + " została zabita.");
	}

	void Entity::setMaxHp(const int max_hp)
	{
		_max_hp = max_hp;
		_hp = max_hp;
		_combat_death_event_emitted = false;
	}

	void Entity::setMaxHpPreservingCurrentHp(const int max_hp)
	{
		_max_hp = max_hp;
		_hp = std::clamp(_hp, 0, _max_hp);
		if (_hp > 0)
			_combat_death_event_emitted = false;
	}

	void Entity::setHP(const int hp)
	{
		_hp = std::clamp(hp, 0, _max_hp);
		_is_dying = false;
		if (_hp > 0)
			_combat_death_event_emitted = false;
		if (_hp > 0 && _type != EntityType::Projectile)
			setFaction(_faction);
	}

	void Entity::setDeathAnimationName(std::string animation_name)
	{
		if (!animation_name.empty())
			_death_anim_name = std::move(animation_name);
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
		const float visual_rotation = (_rotation + _visual_state->modelFacingOffset()) * DEG2RAD;
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

	bool Entity::isVisibleInCamera(const Camera3D& camera, const float screen_margin) const
	{
		if (_dormant)
			return false;

		if (!_model_loaded)
			return DebugColliders;

		const int screen_width = GetScreenWidth();
		const int screen_height = GetScreenHeight();
		if (screen_width <= 0 || screen_height <= 0)
			return true;

		const auto isProjectedOnScreen = [&](const Vector3 point) {
			const Vector2 projected = GetWorldToScreen(point, camera);
			return projected.x >= -screen_margin &&
				projected.x <= static_cast<float>(screen_width) + screen_margin &&
				projected.y >= -screen_margin &&
				projected.y <= static_cast<float>(screen_height) + screen_margin;
		};

		if (isProjectedOnScreen(getWorldPos3D()))
			return true;

		const BoundingBox box = getBoundingBox();
		const Vector3 corners[8] = {
			{box.min.x, box.min.y, box.min.z},
			{box.max.x, box.min.y, box.min.z},
			{box.min.x, box.max.y, box.min.z},
			{box.max.x, box.max.y, box.min.z},
			{box.min.x, box.min.y, box.max.z},
			{box.max.x, box.min.y, box.max.z},
			{box.min.x, box.max.y, box.max.z},
			{box.max.x, box.max.y, box.max.z}
		};

		for (const Vector3& corner : corners) {
			if (isProjectedOnScreen(corner)) {
				return true;
			}
		}

		const Vector3 center = {
			(box.min.x + box.max.x) * 0.5f,
			(box.min.y + box.max.y) * 0.5f,
			(box.min.z + box.max.z) * 0.5f
		};
		return isProjectedOnScreen(center);
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
		const Matrix world_transform = getWorldTransformMatrix();
		const Vector3 corners[] = {
			{local_bb.min.x, local_bb.min.y, local_bb.min.z},
			{local_bb.min.x, local_bb.min.y, local_bb.max.z},
			{local_bb.min.x, local_bb.max.y, local_bb.min.z},
			{local_bb.min.x, local_bb.max.y, local_bb.max.z},
			{local_bb.max.x, local_bb.min.y, local_bb.min.z},
			{local_bb.max.x, local_bb.min.y, local_bb.max.z},
			{local_bb.max.x, local_bb.max.y, local_bb.min.z},
			{local_bb.max.x, local_bb.max.y, local_bb.max.z},
		};

		BoundingBox world_box = {
			Vector3{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()},
			Vector3{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()}
		};

		for (const Vector3& corner : corners) {
			const Vector3 transformed = Vector3Transform(corner, world_transform);
			world_box.min = Vector3Min(world_box.min, transformed);
			world_box.max = Vector3Max(world_box.max, transformed);
		}

		return world_box;
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
		return getAbilityStatsFromConfig(name);
	}

	void Entity::addAbility(const std::shared_ptr<Ability>& ability) 
	{
		_ability_controller->addAbility(*this, ability);
	}

	void Entity::setAbility(const int index, const std::shared_ptr<Ability>& ability)
	{
		_ability_controller->setAbility(*this, index, ability);
	}

	std::shared_ptr<Ability> Entity::getAbility(const int index)
	{
		return _ability_controller->getAbility(index);
	}

	const std::vector<std::shared_ptr<Ability>>& Entity::getAbilities() const
	{
		return _ability_controller->getAbilities();
	}

	void Entity::updateAbilities(const float dt) const 
	{
		_ability_controller->updateAbilities(dt);
	}

	void Entity::addPendingSpawn(std::shared_ptr<Entity> entity)
	{
		_pending_spawn_queue->add(std::move(entity));
	}

	const std::vector<std::shared_ptr<Entity>>& Entity::getPendingSpawns() const
	{
		return _pending_spawn_queue->pending();
	}

	void Entity::clearPendingSpawns()
	{
		_pending_spawn_queue->clear();
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
		if (isMovementRooted()) {
			_velocity = {0.0f, 0.0f};
			_is_moving = false;
			return;
		}

		_target_x = x;
		_target_y = y;

		_is_moving = hasMovementTarget(_pos, {_target_x, _target_y});
	}

	void Entity::stopMovement()
	{
		_velocity = {0.0f, 0.0f};
		_is_moving = false;
	}

	void Entity::setMovementTarget(const float x, const float y)
	{
		_target_x = x;
		_target_y = y;
	}

	void Entity::tickPathRecalcTimer(const float dt)
	{
		_path_recalc_timer -= dt;
	}

	bool Entity::isPathRecalcDue() const
	{
		return _path_recalc_timer <= 0.0f;
	}

	void Entity::resetPathRecalcTimer(const float interval)
	{
		_path_recalc_timer = interval;
	}

	void Entity::clearPathRecalcTimer()
	{
		_path_recalc_timer = 0.0f;
	}

	void Entity::updateMovement(const float dt)
	{
		if (isMovementRooted()) {
			_velocity = {0.0f, 0.0f};
			_is_moving = false;
			return;
		}

		if (!_is_moving) return;

		const float move_dist = _movement_speed * _speed_multiplier * dt;
		const MovementAdvanceResult movement = advanceMovementTowards(_pos, {_target_x, _target_y}, move_dist);
		if (movement.should_face_target)
			rotateTowards(_target_x, _target_y);

		_pos = movement.position;
		_is_moving = movement.moving;
	}

	float Entity::getDistanceToTarget() const
	{
		return TargetingSupport::distanceToTarget(*this, _target);
	}

	Vector2 Entity::getTargetPosition() const
	{
		return TargetingSupport::targetPositionOrSelf(*this, _target);
	}

	bool Entity::hasValidTarget() const
	{
		return TargetingSupport::hasLiveTarget(_target);
	}

	void Entity::chaseTarget(const float dt, const float path_recalc_interval)
	{
		if (!hasValidTarget()) return;
		
		tickPathRecalcTimer(dt);
		
		if (isPathRecalcDue() || !isMoving())
		{
			const Vector2 target_pos = getTargetPosition();
			moveTo(target_pos.x, target_pos.y);
			resetPathRecalcTimer(path_recalc_interval);
		}
		
		updateMovement(dt);
	}

	void Entity::playSoundEffect(const std::string& id, const float volume, const bool restart_if_playing, const float pitch) const
	{
		_audio_controller->playSoundEffect(*this, _audio_manager, id, volume, restart_if_playing, pitch);
	}

	void Entity::stopSoundEffect(const std::string& id) const
	{
		_audio_controller->stopSoundEffect(_audio_manager, id);
	}

	void Entity::updateMovementSound(const std::string& path, const bool should_play, const float volume, const float pitch)
	{
		_audio_controller->updateMovementSound(*this, _audio_manager, path, should_play, volume, pitch);
	}

} // namespace Nawia::Entity
