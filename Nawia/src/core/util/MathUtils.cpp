#include "Map.h"

namespace Nawia::Core {
	struct Point2D {
		float x, y;
	};

	Point2D ScreenToIso(float mouseX, float mouseY, float offsetX, float offsetY) {
		float adjX = mouseX - offsetX;
		float adjY = mouseY - offsetY;

		float halfW = TILE_WIDTH / 2.0f;
		float halfH = TILE_HEIGHT / 2.0f;
		
		float isoY = (adjY / halfH + adjX / halfW) / 2.0f;
		float isoX = (adjY / halfH - adjX / halfW) / 2.0f;

		return { isoX, isoY };
	}
}