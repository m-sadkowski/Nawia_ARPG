#include "AnimationComponent.h"

#include <Constants.h>
#include <Logger.h>
#include <raymath.h>

namespace Nawia::Entity {

	AnimationComponent::AnimationComponent(const std::string& model_path, const bool rotate_model)
	    : _current_anim_index(0), _anim_frame_counter(0), _rotation(0.0f) 
	{
		_model = LoadModel(model_path.c_str());
		if (_model.meshCount == 0) 
			Core::Logger::errorLog("Failed to load model: " + model_path);
		else 
			Core::Logger::debugLog("Loaded model: " + model_path);

		// correction for Z-up models
		if (rotate_model)
			_model.transform = MatrixRotateX(-PI / 2.0f);

		// load animations from the initial file
		addAnimation(model_path);

		// initialize 3D Rendering
		_target = LoadRenderTexture(Core::MODEL_RENDER_SIZE, Core::MODEL_RENDER_SIZE);
		_camera.position = Core::ISOMETRIC_CAMERA_POS;
		_camera.target = Vector3{0.0f, 0.0f, 0.0f};
		_camera.up = Vector3{0.0f, 1.0f, 0.0f};
		_camera.fovy = 45.0f;
		_camera.projection = CAMERA_PERSPECTIVE;
	}

	AnimationComponent::~AnimationComponent() 
	{
		for (const auto &anim : _animations)
			UnloadModelAnimation(anim);

		UnloadModel(_model);
		UnloadRenderTexture(_target);
	}

	void AnimationComponent::addAnimation(const std::string& anim_path)
	{
		int count = 0;
		ModelAnimation* anims = LoadModelAnimations(anim_path.c_str(), &count);

		if (count > 0) 
		{
			Core::Logger::debugLog("Loaded " + std::to_string(count) + " animations from " + anim_path);

			for (int i = 0; i < count; i++)
				_animations.push_back(anims[i]);

			MemFree(anims);
		} 
		else 
		{
			Core::Logger::debugLog("No animations found in " + anim_path);
		}
	}

	void AnimationComponent::update(float dt) 
	{
		if (!_animations.empty()) 
		{
			_anim_frame_counter++;
			UpdateModelAnimation(_model, _animations[_current_anim_index], _anim_frame_counter);

			if (_anim_frame_counter >= _animations[_current_anim_index].frameCount) 
				_anim_frame_counter = 0;
		}
	}

	void AnimationComponent::render(const Vector2 position, const float scale) 
	{
		BeginTextureMode(_target);
		ClearBackground(BLANK);
		BeginMode3D(_camera);

		// Debug visuals
		/*
		DrawGrid(10, 1.0f);
		BoundingBox box = GetModelBoundingBox(_model);
		box.min = Vector3Scale(box.min, scale);
		box.max = Vector3Scale(box.max, scale);
		DrawBoundingBox(box, GREEN);
		*/

		DrawModelEx(_model, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, _rotation, {scale, scale, scale}, WHITE);
		EndMode3D();
		EndTextureMode();

		const float texture_width = static_cast<float>(_target.texture.width);
		const float texture_height = static_cast<float>(_target.texture.height);

		const Rectangle source = {0.0f, 0.0f, texture_width, -texture_height };
		const Rectangle dest = {position.x, position.y, texture_width, texture_height };
		const Vector2 origin = {dest.width / 2.0f, dest.height / 2.0f };

		DrawTexturePro(_target.texture, source, dest, origin, 0.0f, WHITE);
	}

	void AnimationComponent::playAnimation(const int index) 
	{
		if (index >= 0 && index < static_cast<int>(_animations.size()) && index != _current_anim_index) 
		{
			_current_anim_index = index;
			_anim_frame_counter = 0;
		}
	}

} // namespace Nawia::Entity
