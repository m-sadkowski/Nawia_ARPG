#pragma once

#include <Entity.h>

namespace Nawia::Entity {

	class HerbalistHub : public Entity {
	public:
		HerbalistHub(const std::string& name, float x, float y, float radius);

		[[nodiscard]] float getRadius() const { return _radius; }
		void render(const Camera3D& camera) override;

	private:
		float _radius = 5.0f;
	};

} // namespace Nawia::Entity
