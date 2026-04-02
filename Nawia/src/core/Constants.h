#pragma once

#include <raylib.h>

namespace Nawia::Core
{
	constexpr int WINDOW_WIDTH = 1280;
	constexpr int WINDOW_HEIGHT = 720;

	// Base/reference resolution for UI scaling (design-time resolution)
	constexpr int BASE_WIDTH = 1920;
	constexpr int BASE_HEIGHT = 1080;

	// 3D Camera defaults (isometric-like angle)
	constexpr float CAMERA_DISTANCE = 25.0f;
	constexpr float CAMERA_HEIGHT = 20.0f;
	constexpr float CAMERA_FOV = 45.0f;
}
