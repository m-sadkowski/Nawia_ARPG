#pragma once

#include <raylib.h>

#include <vector>

namespace Nawia::Entity {

	class EntityVisualState {
	public:
		void setModelTint(Color tint) { _model_tint = tint; }
		[[nodiscard]] Color modelTint() const { return _model_tint; }

		void setHovered(bool hovered) { _hovered = hovered; }
		[[nodiscard]] bool hovered() const { return _hovered; }

		void setModelFacingOffset(float deg) { _model_facing_offset = deg; }
		[[nodiscard]] float modelFacingOffset() const { return _model_facing_offset; }

		void hideMeshIndex(int mesh_index);
		[[nodiscard]] bool hasHiddenMeshes() const { return !_hidden_mesh_indices.empty(); }
		[[nodiscard]] bool isMeshHidden(int mesh_index) const;

	private:
		float _model_facing_offset = 90.0f;
		Color _model_tint = WHITE;
		bool _hovered = false;
		std::vector<int> _hidden_mesh_indices;
	};

}
