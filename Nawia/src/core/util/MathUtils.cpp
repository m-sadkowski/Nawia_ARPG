#include "MathUtils.h"

namespace Nawia::Core {
	Point2D Point2D::screenToIso(float mouseX, float mouseY) {
		float adjX = mouseX - MAP_OFFSET_X;
		float adjY = mouseY - MAP_OFFSET_Y;

		float halfW = TILE_WIDTH / 2.0f;
		float halfH = TILE_HEIGHT / 2.0f;
		
		float isoY = (adjY / halfH - adjX / halfW) / 2.0f;
		float isoX = (adjY / halfH + adjX / halfW) / 2.0f;

		return { isoX, isoY };
	}
}