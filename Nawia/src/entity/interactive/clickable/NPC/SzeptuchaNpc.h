#pragma once

#include <StoryNpc.h>

namespace Nawia::Entity {

	class SzeptuchaNpc : public StoryNpc {
	public:
		SzeptuchaNpc(const std::string& name, float x, float y, Core::Engine* engine);
		void update(float delta_time) override;
		[[nodiscard]] Vector3 getWorldPos3D() const override;
		[[nodiscard]] const char* getNpcClass() const override { return "szeptucha"; }

	private:
		[[nodiscard]] float getIdleBobOffset() const;
		void updateIdleVisualTransform();

		Matrix _base_model_transform = {};
		float _idle_visual_time = 0.0f;
		bool _has_base_model_transform = false;
	};

} // namespace Nawia::Entity
