#pragma once

#include <Stats.h>

#include <json.hpp>
#include <raylib.h>

namespace Nawia::Entity::PlayerDetail {

	constexpr const char* PLAYER_HEAD_MODEL = "assets/models/actors/player/parts/player_head.glb";
	constexpr const char* PLAYER_HEAD_WITH_SWORD_MODEL = "assets/models/items/player_head_with_sword.glb";
	constexpr const char* FIREBALL_MODEL = "assets/models/fireball.glb";
	constexpr const char* FIREBALL_ICON = "assets/textures/icons/fireball_icon.png";
	constexpr int FIREBALL_ABILITY_SLOT = 2;

	nlohmann::json statsToJson(const Stats& stats);
	Stats statsFromJson(const nlohmann::json& data, const Stats& fallback);
	nlohmann::json vector2ToJson(Vector2 value);
	Vector2 vector2FromJson(const nlohmann::json& data, Vector2 fallback);

} // namespace Nawia::Entity::PlayerDetail
