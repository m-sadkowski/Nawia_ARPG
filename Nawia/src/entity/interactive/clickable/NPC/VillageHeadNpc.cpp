#include "VillageHeadNpc.h"

namespace Nawia::Entity {

	VillageHeadNpc::VillageHeadNpc(
		const std::string& name,
		const float x,
		const float y,
		Core::Engine* engine)
		: StoryNpc(name, x, y)
	{
		configureVillageHead(engine);
	}

} // namespace Nawia::Entity
