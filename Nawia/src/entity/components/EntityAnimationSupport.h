#pragma once

#include <Entity.h>

#include <memory>
#include <string>

namespace Nawia::Entity {

	void ensureMeshAnimationBuffers(Model& model);
	[[nodiscard]] std::shared_ptr<const AnimationBundle> getCachedAnimationBundle(const std::string& path);
	[[nodiscard]] const ModelAnimation* resolveAnimation(const AnimationClipRef& animation);

}
