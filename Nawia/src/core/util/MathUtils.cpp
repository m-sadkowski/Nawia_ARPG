#include "MathUtils.h"

#include <Map.h>

namespace Nawia::Core {

	Point2D Point2D::screenToIso(const float mouse_x, const float mouse_y) {
		const float adj_x = mouse_x - MAP_OFFSET_X;
		const float adj_y = mouse_y - MAP_OFFSET_Y;

		constexpr float half_width = TILE_WIDTH / 2.0f;
		constexpr float half_height = TILE_HEIGHT / 2.0f;
		
		const float iso_x = (adj_y / half_height + adj_x / half_width) / 2.0f;
		const float iso_y = (adj_y / half_height - adj_x / half_width) / 2.0f;

		return { iso_x, iso_y };
	}

} // namespace Nawia::Core
