#include "MathUtils.h"

#include <Map.h>

namespace Nawia::Core {

	Point2D Point2D::screenToIso(float mouse_x, float mouse_y, float offsetX, float offsetY) {
		float adj_x = mouse_x - offsetX;
		const float adj_y = mouse_y - offsetY;

		constexpr float half_width = TILE_WIDTH / 2.0f;
		constexpr float half_height = TILE_HEIGHT / 2.0f;

		adj_x -= half_width;
		
		const float iso_x = (adj_y / half_height + adj_x / half_width) / 2.0f;
		const float iso_y = (adj_y / half_height - adj_x / half_width) / 2.0f;

		return { iso_x, iso_y };
	}

} // namespace Nawia::Core
