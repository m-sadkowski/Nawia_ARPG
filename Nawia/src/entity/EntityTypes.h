#pragma once

#include <raylib.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Nawia::Entity {

	class Entity;

	using EntityId = std::uint64_t;
	inline constexpr EntityId INVALID_ENTITY_ID = 0;

	enum class EntityType {
		None,
		Player,
		Enemy,
		Ally,
		NPCActor,
		NPCStatic,
		Hazard,
		Projectile,
		Trigger,
		Chest,
		Item
	};

	enum class Faction {
		Player,
		Enemy,
		Neutral,
		Ally,
		None
	};

	struct DamageSourceContext {
		bool valid = false;
		std::weak_ptr<Entity> source;
		EntityId source_id = INVALID_ENTITY_ID;
		std::string source_name;
		EntityType source_type = EntityType::None;
		Faction source_faction = Faction::None;
		Vector2 source_position = {0.0f, 0.0f};
		std::string label;
	};

	struct EntityCastState {
		bool active = false;
		std::string name;
		float duration_seconds = 0.0f;
		float remaining_seconds = 0.0f;
		bool interruptible = false;
	};

	struct AnimationBundle {
		std::vector<ModelAnimation> clips;
		~AnimationBundle();
	};

	struct AnimationClipRef {
		std::shared_ptr<const AnimationBundle> bundle;
		int clip_index = 0;
	};

} // namespace Nawia::Entity
