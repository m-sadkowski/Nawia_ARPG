#pragma once

#include <StoryNpc.h>

namespace Nawia::Entity {

	class MushroomNpc : public StoryNpc {
	public:
		MushroomNpc(
			const std::string& name,
			float x,
			float y,
			Core::Engine* engine,
			const std::string& follow_checkpoint_name = "Checkpoint Gziba");
		[[nodiscard]] const char* getNpcClass() const override { return "mushroom"; }
	};

} // namespace Nawia::Entity
