#include "HerbalistHub.h"

#include <algorithm>
#include <raymath.h>

namespace Nawia::Entity {

	HerbalistHub::HerbalistHub(
		const std::string& name,
		const float x,
		const float y,
		const float radius)
		: Entity(name, x, y, nullptr, 1),
		  _radius(std::max(0.1f, radius))
	{
		setType(EntityType::NPCStatic);
		setFaction(Faction::None);
	}

	void HerbalistHub::render(const Camera3D& camera) {
		(void)camera;
		if (!DebugColliders)
			return;

		const Vector3 center = {getX(), getAltitude() + 0.05f, getY()};
		DrawCircle3D(center, _radius, {1.0f, 0.0f, 0.0f}, 90.0f, ColorAlpha(GREEN, 0.28f));
		DrawCylinderWires(center, _radius, _radius, 0.24f, 36, GREEN);
	}

} // namespace Nawia::Entity
