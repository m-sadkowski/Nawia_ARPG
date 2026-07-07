#include "Entity.h"

#include <EntityAnimationSupport.h>
#include <EntityModelState.h>

#include <algorithm>

namespace Nawia::Entity {

	void Entity::addAnimation(const std::string& name, const std::string& path)
	{
		addAnimation(name, path, 0);
	}

	void Entity::addAnimation(const std::string& name, const std::string& path, const int clip_index)
	{
		if (!_model_state->model_loaded)
			return;

		if (const auto cached_path = _model_state->animation_path_map.find(path); cached_path != _model_state->animation_path_map.end())
		{
			const int animation_index = cached_path->second + clip_index;
			if (clip_index >= 0 && static_cast<size_t>(animation_index) < _model_state->animations.size())
				_model_state->animation_map[name] = animation_index;
			return;
		}

		const auto bundle = getCachedAnimationBundle(path);
		if (!bundle || bundle->clips.empty() || clip_index < 0 || static_cast<size_t>(clip_index) >= bundle->clips.size())
			return;

		const int start_index = static_cast<int>(_model_state->animations.size());

		for (int i = 0; i < static_cast<int>(bundle->clips.size()); i++)
			_model_state->animations.push_back({bundle, i});

		_model_state->animation_map[name] = start_index + clip_index;
		_model_state->animation_path_map[path] = start_index;
	}

	void Entity::loadAnimationBundle(const std::string& path) {
		if (!_model_state->model_loaded)
			return;

		const auto bundle = getCachedAnimationBundle(path);
		if (!bundle || bundle->clips.empty())
			return;

		if (const auto cached_path = _model_state->animation_path_map.find(path); cached_path != _model_state->animation_path_map.end()) {
			const int start_index = cached_path->second;
			for (int i = 0; i < static_cast<int>(bundle->clips.size()); i++) {
				std::string anim_name = bundle->clips[i].name;
				if (anim_name.empty())
					anim_name = "anim_" + std::to_string(start_index + i);

				_model_state->animation_map[anim_name] = start_index + i;
			}
			return;
		}

		const int start_index = static_cast<int>(_model_state->animations.size());

		for (int i = 0; i < static_cast<int>(bundle->clips.size()); i++) {
			_model_state->animations.push_back({bundle, i});

			std::string anim_name = bundle->clips[i].name;
			if (anim_name.empty())
				anim_name = "anim_" + std::to_string(start_index + i);

			_model_state->animation_map[anim_name] = start_index + i;
		}

		_model_state->animation_path_map[path] = start_index;
	}

	void Entity::preloadAnimationData(const std::string& path)
	{
		if (!path.empty())
			(void)getCachedAnimationBundle(path);
	}

	void Entity::playAnimation(const std::string& name, const bool loop, const bool lock_movement, const int start_frame, const bool force)
	{
		const auto animation_it = _model_state->animation_map.find(name);
		if (animation_it == _model_state->animation_map.end())
			return;

		const int index = animation_it->second;
		if (!force && index == _model_state->current_anim_index)
			return;

		_model_state->current_anim_index = index;
		_model_state->anim_frame_counter = static_cast<float>(start_frame);
		_model_state->anim_looping = loop;
		_model_state->anim_locked = lock_movement;
		_model_state->anim_ping_pong = false;
		_model_state->anim_reverse_phase = false;
		_model_state->freeze_animation_on_completion = false;
		_model_state->animation_frozen_at_last_frame = false;
		_model_state->anim_direction = 1.0f;
		_model_state->last_applied_anim_index = -1;
		_model_state->last_applied_anim_frame = -1;
		applyCurrentAnimationFrame();
	}

	void Entity::playAnimationFreezeOnLastFrame(
		const std::string& name,
		const bool lock_movement,
		const int start_frame,
		const bool force)
	{
		playAnimation(name, false, lock_movement, start_frame, force);
		if (_model_state->animation_map.find(name) != _model_state->animation_map.end())
			_model_state->freeze_animation_on_completion = true;
	}

	void Entity::playAnimationPingPong(
		const std::string& name,
		const bool lock_movement,
		const int start_frame,
		const bool force)
	{
		playAnimation(name, false, lock_movement, start_frame, force);

		if (_model_state->animation_map.find(name) == _model_state->animation_map.end())
			return;

		_model_state->anim_ping_pong = true;
		_model_state->anim_reverse_phase = false;
		_model_state->anim_direction = 1.0f;
	}

	int Entity::getAnimationFrameCount(const std::string& name) const
	{
		const auto animation_it = _model_state->animation_map.find(name);
		if (animation_it == _model_state->animation_map.end())
			return 0;

		const int index = animation_it->second;
		if (index < 0 || static_cast<size_t>(index) >= _model_state->animations.size())
			return 0;

		const ModelAnimation* animation = resolveAnimation(_model_state->animations[index]);
		return animation ? animation->frameCount : 0;
	}

	bool Entity::hasAnimationReachedFrame(const float frame) const
	{
		return _model_state->anim_frame_counter >= frame;
	}

	bool Entity::hasAnimationReachedRatio(const std::string& animation_name, const float frame_ratio) const
	{
		const int frame_count = getAnimationFrameCount(animation_name);
		return frame_count > 0 && hasAnimationReachedFrame(static_cast<float>(frame_count) * frame_ratio);
	}

	bool Entity::consumeAnimationFrameTrigger(
		const std::string& animation_name,
		const float frame_ratio,
		bool& consumed) const
	{
		if (consumed || !hasAnimationReachedRatio(animation_name, frame_ratio))
			return false;

		consumed = true;
		return true;
	}

	void Entity::setAnimationSpeed(const float multiplier)
	{
		_model_state->anim_speed_multiplier = multiplier;
	}

	float Entity::getAnimationSpeed() const
	{
		return _model_state->anim_speed_multiplier;
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
		_model_state->anim_direction = -1.0f;
		_model_state->anim_looping = false;
		_model_state->anim_locked = lock_movement;
		_model_state->anim_ping_pong = false;
		_model_state->anim_reverse_phase = false;
		_model_state->last_applied_anim_index = -1;
		_model_state->last_applied_anim_frame = -1;
		applyCurrentAnimationFrame();
	}

	bool Entity::advanceAnimationTowardFrame(const float dt, const int target_frame, const bool lock_when_reached)
	{
		const float final_frame = static_cast<float>(std::max(0, target_frame));
		_model_state->anim_frame_counter += dt * _model_state->anim_fps * getAnimationSpeed() / ANIMATION_DURATION_SCALE;
		if (_model_state->anim_frame_counter >= final_frame) {
			_model_state->anim_frame_counter = final_frame;
			_model_state->anim_locked = lock_when_reached;
			applyCurrentAnimationFrame();
			return true;
		}

		applyCurrentAnimationFrame();
		return false;
	}

	void Entity::applyCurrentAnimationFrame()
	{
		if (!_model_state->model_loaded || _model_state->animations.empty())
			return;

		if (_model_state->current_anim_index < 0 || static_cast<size_t>(_model_state->current_anim_index) >= _model_state->animations.size())
			return;

		const ModelAnimation* current_animation = resolveAnimation(_model_state->animations[_model_state->current_anim_index]);
		if (!current_animation || current_animation->frameCount <= 0)
			return;

		for (int i = 0; i < _model_state->model.meshCount; i++) {
			if (_model_state->model.meshes[i].boneWeights == nullptr || _model_state->model.meshes[i].boneIds == nullptr)
				return;
		}

		const int animation_frame = std::clamp(
			static_cast<int>(_model_state->anim_frame_counter),
			0,
			current_animation->frameCount - 1
		);
		if (_model_state->last_applied_anim_index == _model_state->current_anim_index && _model_state->last_applied_anim_frame == animation_frame)
			return;

		UpdateModelAnimation(_model_state->model, *current_animation, animation_frame);
		updateAttachedModelAnimation(*current_animation, animation_frame);

		_model_state->last_applied_anim_index = _model_state->current_anim_index;
		_model_state->last_applied_anim_frame = animation_frame;
	}

	void Entity::updateAnimation(const float dt)
	{
		if (!_model_state->model_loaded || _model_state->animations.empty())
			return;

		if (_model_state->current_anim_index < 0 || static_cast<size_t>(_model_state->current_anim_index) >= _model_state->animations.size())
			return;

		const ModelAnimation* current_animation = resolveAnimation(_model_state->animations[_model_state->current_anim_index]);
		if (!current_animation || current_animation->frameCount <= 0)
			return;

		if (_model_state->animation_frozen_at_last_frame)
			return;

		_model_state->anim_frame_counter += dt * _model_state->anim_fps * _model_state->anim_speed_multiplier * _model_state->anim_direction / ANIMATION_DURATION_SCALE;

		if (_model_state->anim_frame_counter >= current_animation->frameCount)
		{
			if (_model_state->anim_looping) {
				while (_model_state->anim_frame_counter >= current_animation->frameCount)
					_model_state->anim_frame_counter -= current_animation->frameCount;
			}
			else if (_model_state->anim_ping_pong && !_model_state->anim_reverse_phase) {
				_model_state->anim_reverse_phase = true;
				_model_state->anim_direction = -1.0f;
				_model_state->anim_frame_counter = static_cast<float>(current_animation->frameCount - 1);
			}
			else if (_model_state->freeze_animation_on_completion) {
				_model_state->freeze_animation_on_completion = false;
				_model_state->anim_frame_counter = static_cast<float>(current_animation->frameCount - 1);
				_model_state->anim_locked = false;
				_model_state->animation_frozen_at_last_frame = true;
			}
			else {
				_model_state->anim_frame_counter = 0;
				if (getAnimationFrameCount("Idle_Loop") > 0)
					playAnimation("Idle_Loop", true, false, 0, true);
				else if (getAnimationFrameCount("Idle") > 0)
					playAnimation("Idle", true, false, 0, true);
				else if (getAnimationFrameCount("idle") > 0)
					playAnimation("idle", true, false, 0, true);
				else
					playAnimation("default", true, false, 0, true);

				current_animation = resolveAnimation(_model_state->animations[_model_state->current_anim_index]);
				if (!current_animation || current_animation->frameCount <= 0)
					return;
			}
		}
		else if (_model_state->anim_frame_counter <= 0.0f && _model_state->anim_direction < 0.0f)
		{
			_model_state->anim_frame_counter = 0.0f;
			_model_state->anim_ping_pong = false;
			_model_state->anim_reverse_phase = false;
			_model_state->anim_direction = 1.0f;

			if (getAnimationFrameCount("Idle_Loop") > 0)
				playAnimation("Idle_Loop", true, false, 0, true);
			else if (getAnimationFrameCount("Idle") > 0)
				playAnimation("Idle", true, false, 0, true);
			else if (getAnimationFrameCount("idle") > 0)
				playAnimation("idle", true, false, 0, true);
			else
				playAnimation("default", true, false, 0, true);

			current_animation = resolveAnimation(_model_state->animations[_model_state->current_anim_index]);
			if (!current_animation || current_animation->frameCount <= 0)
				return;
		}

		applyCurrentAnimationFrame();
	}

	bool Entity::isAnimationLocked() const
	{
		return _model_state->anim_locked;
	}

} // namespace Nawia::Entity
