#pragma once

#include <StoryNpc.h>

namespace Nawia::Entity {

	/**
	 * @brief Fabularna Baba Yaga uzywana w sekwencji intro Wczora.
	 *
	 * Track animacji GLB jest obecnie niestabilny, wiec klasa trzyma model
	 * statyczny i naklada delikatny proceduralny idle przez bob/pochylenie.
	 */
	class SzeptuchaNpc : public StoryNpc {
	public:
		SzeptuchaNpc(const std::string& name, float x, float y, Core::Engine* engine);
		void update(float delta_time) override;
		[[nodiscard]] Vector3 getWorldPos3D() const override;
		[[nodiscard]] const char* getNpcClass() const override { return "szeptucha"; }

	private:
		/** @brief Aktualny pionowy offset wizualny uzywany przez sztuczny idle. */
		[[nodiscard]] float getIdleBobOffset() const;
		void updateIdleVisualTransform();

		Matrix _base_model_transform = {}; ///< Transformacja fit/scale przed proceduralnym idle.
		float _idle_visual_time = 0.0f;
		bool _has_base_model_transform = false;
	};

} // namespace Nawia::Entity
