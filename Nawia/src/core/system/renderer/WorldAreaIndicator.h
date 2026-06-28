#pragma once

#include <raylib.h>

namespace Nawia::Core::System::Renderer {

	/**
	 * @struct GroundDiscStyle
	 * @brief Visual settings for smooth world-space circular area markers.
	 */
	struct GroundDiscStyle {
		float radius = 1.0f;
		float height = 0.08f;
		float core_radius_fraction = 0.72f;
		float core_height_fraction = 0.45f;
		float core_vertical_fraction = 0.35f;
		int slices = 96;
		Color fill_color = WHITE;
		Color core_color = WHITE;
	};

	/**
	 * @struct GroundRingStyle
	 * @brief Visual settings for smooth expanding rings, such as shockwaves.
	 */
	struct GroundRingStyle {
		float inner_radius = 0.0f;
		float outer_radius = 1.0f;
		int slices = 128;
		Color color = WHITE;
	};

	/**
	 * @brief Draws a smooth filled ground disc without wireframe edges.
	 *
	 * Gameplay telegraphs and pings should use this instead of
	 * DrawCylinderWires(), because wire cylinders read as angular boxes from the
	 * isometric camera.
	 */
	void drawSoftGroundDisc(Vector3 center, const GroundDiscStyle& style);
	void drawSoftGroundRing(Vector3 center, const GroundRingStyle& style);

} // namespace Nawia::Core::System::Renderer
