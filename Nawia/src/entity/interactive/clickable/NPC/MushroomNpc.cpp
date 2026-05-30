#include "MushroomNpc.h"

namespace Nawia::Entity {

	MushroomNpc::MushroomNpc(
		const std::string& name,
		const float x,
		const float y,
		Core::Engine* engine,
		const std::string& follow_checkpoint_name)
		: StoryNpc(name, x, y)
	{
		configureMushroom(engine, follow_checkpoint_name);
	}

} // namespace Nawia::Entity
