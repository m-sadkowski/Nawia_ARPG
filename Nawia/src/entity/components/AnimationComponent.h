#pragma once
#include "raylib.h"
#include <string>
#include <vector>

namespace Nawia::Entity {

	class AnimationComponent {
	public:
		AnimationComponent(const std::string& model_path, bool rotate_model = false);
		~AnimationComponent();

		void update(float dt);
		// render model to a texture and draw texture at 2D screen position
		void render(Vector2 position, float scale = 1.0f);

		void playAnimation(int index);
		void addAnimation(const std::string& anim_path);
		void setRotation(const float angle) { _rotation = angle; } // Y-axis rotation

	private:
		Model _model;
		std::vector<ModelAnimation> _animations;
		int _current_anim_index;
		int _anim_frame_counter;
		float _rotation;

		// 3D rendering support
		RenderTexture2D _target;
		Camera3D _camera;
	};

} // namespace Nawia::Entity
