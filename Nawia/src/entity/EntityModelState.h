#pragma once

#include <EntityTypes.h>

#include <raylib.h>

#include <map>
#include <string>
#include <vector>

namespace Nawia::Entity {

	struct EntityModelState {
		Model model = {};
		std::vector<AnimationClipRef> animations;
		std::map<std::string, int> animation_map;
		std::map<std::string, int> animation_path_map;

		int current_anim_index = 0;
		float anim_frame_counter = 0.0f;
		int last_applied_anim_index = -1;
		int last_applied_anim_frame = -1;
		float anim_speed_multiplier = 1.0f;
		float anim_fps = 60.0f;
		bool model_loaded = false;
		bool owns_model = false;
		bool cloned_model = false;
		BoundingBox local_model_bounding_box = {};
		bool local_model_bounding_box_valid = false;
		bool anim_looping = true;
		bool anim_locked = false;
		bool anim_ping_pong = false;
		bool anim_reverse_phase = false;
		bool freeze_animation_on_completion = false;
		bool animation_frozen_at_last_frame = false;
		float anim_direction = 1.0f;
	};

} // namespace Nawia::Entity
