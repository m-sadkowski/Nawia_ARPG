#include "WorldAreaIndicator.h"

#include <algorithm>

namespace Nawia::Core::System::Renderer {

	void drawSoftGroundDisc(const Vector3 center, const GroundDiscStyle& style) {
		const float radius = std::max(0.01f, style.radius);
		const float height = std::max(0.01f, style.height);
		const int slices = std::max(32, style.slices);

		if (style.fill_color.a > 0)
			DrawCylinder(center, radius, radius, height, slices, style.fill_color);

		if (style.core_color.a <= 0)
			return;

		const float core_radius_fraction = std::clamp(style.core_radius_fraction, 0.0f, 1.0f);
		if (core_radius_fraction <= 0.0f)
			return;

		const float core_height_fraction = std::clamp(style.core_height_fraction, 0.0f, 1.0f);
		const float core_vertical_fraction = std::clamp(style.core_vertical_fraction, 0.0f, 1.0f);
		const float core_height = std::max(0.01f, height * core_height_fraction);
		const Vector3 core_center = {
			center.x,
			center.y + height * core_vertical_fraction,
			center.z
		};

		DrawCylinder(
			core_center,
			std::max(0.01f, radius * core_radius_fraction),
			std::max(0.01f, radius * core_radius_fraction),
			core_height,
			slices,
			style.core_color);
	}

} // namespace Nawia::Core::System::Renderer
