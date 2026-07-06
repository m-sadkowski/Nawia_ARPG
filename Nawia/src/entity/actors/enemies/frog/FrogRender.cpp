#include "Frog.h"

#include "FrogInternal.h"

#include <raymath.h>

#include <cmath>

namespace Nawia::Entity {

	namespace {

		Vector3 offsetByLateral(const Vector3& point, const Vector3& lateral, const float amount) {
			return {
				point.x + lateral.x * amount,
				point.y + lateral.y * amount,
				point.z + lateral.z * amount
			};
		}

		void drawTongueRibbon(
			const Vector3& origin,
			const Vector3& end,
			const float origin_half_width,
			const float end_half_width,
			const Color fill_color,
			const Color edge_color)
		{
			const float dx = end.x - origin.x;
			const float dz = end.z - origin.z;
			const float length = std::sqrt(dx * dx + dz * dz);
			if (length <= 0.001f)
				return;

			const Vector3 lateral = {-dz / length, 0.0f, dx / length};
			const Vector3 origin_left = offsetByLateral(origin, lateral, origin_half_width);
			const Vector3 origin_right = offsetByLateral(origin, lateral, -origin_half_width);
			const Vector3 end_left = offsetByLateral(end, lateral, end_half_width);
			const Vector3 end_right = offsetByLateral(end, lateral, -end_half_width);

			DrawTriangle3D(origin_left, end_left, end_right, fill_color);
			DrawTriangle3D(origin_left, end_right, origin_right, fill_color);
			DrawLine3D(origin_left, end_left, edge_color);
			DrawLine3D(origin_right, end_right, edge_color);
		}

	}

	void Frog::render(const Camera3D& camera) {
		Entity::render(camera);

		if (_special_state != SpecialState::TongueWindup &&
			_special_state != SpecialState::TonguePull &&
			_special_state != SpecialState::TongueRecover) {
			return;
		}

		const Vector2 frog_center = getCenter();
		Vector2 end_2d = _tongue_target_snapshot;
		if (const auto victim = _tongue_victim.lock())
			end_2d = victim->getCenter();

		Vector2 aim = Vector2Subtract(end_2d, frog_center);
		if (Vector2LengthSqr(aim) <= 0.001f)
			aim = getTongueAimDirection();
		else
			aim = Vector2Normalize(aim);

		const Vector2 origin_2d = {
			frog_center.x + aim.x * FrogTuning::TONGUE_MOUTH_FORWARD_OFFSET,
			frog_center.y + aim.y * FrogTuning::TONGUE_MOUTH_FORWARD_OFFSET
		};

		if (_special_state == SpecialState::TongueWindup) {
			end_2d = {
				origin_2d.x + aim.x * (FrogTuning::TONGUE_MAX_RANGE - FrogTuning::TONGUE_MOUTH_FORWARD_OFFSET),
				origin_2d.y + aim.y * (FrogTuning::TONGUE_MAX_RANGE - FrogTuning::TONGUE_MOUTH_FORWARD_OFFSET)
			};
		}

		const float height = getAltitude() + 1.05f;
		const Vector3 origin = {origin_2d.x, height, origin_2d.y};
		const Vector3 end = {end_2d.x, height, end_2d.y};

		if (_special_state == SpecialState::TongueWindup) {
			drawTongueRibbon(
				origin,
				end,
				FrogTuning::TONGUE_TELEGRAPH_BASE_HALF_WIDTH,
				FrogTuning::TONGUE_TELEGRAPH_TIP_HALF_WIDTH,
				Fade(RED, 0.18f),
				Fade(RED, 0.52f)
			);
			DrawCylinderEx(origin, end, 0.035f, 0.025f, 8, Fade(ORANGE, 0.62f));
			return;
		}

		const Vector3 raised_origin = {origin.x, origin.y + 0.03f, origin.z};
		const Vector3 raised_end = {end.x, end.y + 0.03f, end.z};
		drawTongueRibbon(
			origin,
			end,
			FrogTuning::TONGUE_VISUAL_BASE_HALF_WIDTH,
			FrogTuning::TONGUE_VISUAL_TIP_HALF_WIDTH,
			Fade(Color{196, 35, 66, 255}, 0.82f),
			Fade(MAROON, 0.82f)
		);
		DrawCylinderEx(raised_origin, raised_end, 0.11f, 0.15f, 12, Fade(Color{255, 103, 132, 255}, 0.88f));
		const Vector3 tip_highlight = {end.x, end.y + 0.04f, end.z};
		DrawSphere(end, _special_state == SpecialState::TonguePull ? 0.34f : 0.26f, Fade(Color{205, 38, 70, 255}, 0.94f));
		DrawSphere(tip_highlight, 0.13f, Fade(Color{255, 150, 165, 255}, 0.72f));
	}

} // namespace Nawia::Entity
