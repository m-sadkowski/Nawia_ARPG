#pragma once
#include <raylib.h>

namespace Nawia::Core
{

	constexpr int WINDOW_WIDTH = 1280;
	constexpr int WINDOW_HEIGHT = 720;

	// Base/reference resolution for UI scaling (design-time resolution)
	constexpr int BASE_WIDTH = 1920;
	constexpr int BASE_HEIGHT = 1080;

	constexpr int TILE_WIDTH = 128;
	constexpr int TILE_HEIGHT = 64;

	constexpr int ENTITY_TEXTURE_WIDTH = 128;
	constexpr int ENTITY_TEXTURE_HEIGHT = 64;

	constexpr int MODEL_RENDER_SIZE = 256*2;
	constexpr Vector3 ISOMETRIC_CAMERA_POS = Vector3{ -10.0f, 10.0f, 10.0f };

}
