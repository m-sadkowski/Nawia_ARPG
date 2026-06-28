#include "WorldAreaIndicator.h"

#include <algorithm>
#include <cmath>

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

	void drawSoftGroundRing(const Vector3 center, const GroundRingStyle& style) {
		if (style.color.a <= 0)
			return;

		const float outer_radius = std::max(0.01f, style.outer_radius);
		const float inner_radius = std::clamp(style.inner_radius, 0.0f, outer_radius);
		if (inner_radius <= 0.01f) {
			GroundDiscStyle disc_style;
			disc_style.radius = outer_radius;
			disc_style.height = 0.08f;
			disc_style.core_color = {0, 0, 0, 0};
			disc_style.fill_color = style.color;
			disc_style.slices = style.slices;
			drawSoftGroundDisc(center, disc_style);
			return;
		}

		const int slices = std::max(32, style.slices);
		for (int i = 0; i < slices; ++i) {
			const float a0 = (static_cast<float>(i) / static_cast<float>(slices)) * 2.0f * PI;
			const float a1 = (static_cast<float>(i + 1) / static_cast<float>(slices)) * 2.0f * PI;
			const Vector3 outer0 = {
				center.x + std::cos(a0) * outer_radius,
				center.y,
				center.z + std::sin(a0) * outer_radius
			};
			const Vector3 outer1 = {
				center.x + std::cos(a1) * outer_radius,
				center.y,
				center.z + std::sin(a1) * outer_radius
			};
			const Vector3 inner0 = {
				center.x + std::cos(a0) * inner_radius,
				center.y,
				center.z + std::sin(a0) * inner_radius
			};
			const Vector3 inner1 = {
				center.x + std::cos(a1) * inner_radius,
				center.y,
				center.z + std::sin(a1) * inner_radius
			};

			DrawTriangle3D(outer0, inner0, inner1, style.color);
			DrawTriangle3D(outer0, inner1, outer1, style.color);
			DrawTriangle3D(outer0, inner1, inner0, style.color);
			DrawTriangle3D(outer0, outer1, inner1, style.color);
		}
	}

} // namespace Nawia::Core::System::Renderer
