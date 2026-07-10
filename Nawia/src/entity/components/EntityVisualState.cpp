#include "EntityVisualState.h"

#include <algorithm>

namespace Nawia::Entity {

	void EntityVisualState::hideMeshIndex(const int mesh_index)
	{
		if (mesh_index < 0)
			return;

		if (std::find(_hidden_mesh_indices.begin(), _hidden_mesh_indices.end(), mesh_index) == _hidden_mesh_indices.end())
			_hidden_mesh_indices.push_back(mesh_index);
	}

	bool EntityVisualState::isMeshHidden(const int mesh_index) const
	{
		return std::find(_hidden_mesh_indices.begin(), _hidden_mesh_indices.end(), mesh_index) != _hidden_mesh_indices.end();
	}

}
