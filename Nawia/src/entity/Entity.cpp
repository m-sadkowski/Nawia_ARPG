#include "Entity.h"

#include <Collider.h>
#include <EntityAbilityController.h>
#include <EntityAudioController.h>
#include <EntityPendingSpawnQueue.h>
#include <EntityStatusController.h>
#include <EntityVisualState.h>

#include <utility>

namespace Nawia::Entity {

namespace {
	Core::ResourceManager* g_shared_resource_manager = nullptr;
	Game::CombatEventBus* g_combat_event_bus = nullptr;
}

bool Entity::DebugColliders = false;

	Entity::Entity()
		: _pos{0.0f, 0.0f}, _velocity{0.0f, 0.0f}, _scale(1.0f),
		  _hp(1), _max_hp(1), _type(EntityType::None), _faction(Faction::None),
		  _ability_controller(std::make_unique<EntityAbilityController>()),
		  _audio_controller(std::make_unique<EntityAudioController>()),
		  _pending_spawn_queue(std::make_unique<EntityPendingSpawnQueue>()),
		  _status_controller(std::make_unique<EntityStatusController>()),
		  _visual_state(std::make_unique<EntityVisualState>()) {}

	Entity::Entity(
		const std::string& name,
		const float start_x,
		const float start_y,
		const std::shared_ptr<Texture2D>& texture,
		const int max_hp)
		: _name(name), _texture(texture), _max_hp(max_hp), _hp(max_hp),
		  _current_anim_index(0), _anim_frame_counter(0.0f), _rotation(0.0f), _model_loaded(false),
		  _velocity{0.0f, 0.0f}, _scale(1.0f), _faction(Faction::None), _pos{start_x, start_y},
		  _anim_looping(true), _anim_locked(false),
		  _ability_controller(std::make_unique<EntityAbilityController>()),
		  _audio_controller(std::make_unique<EntityAudioController>()),
		  _pending_spawn_queue(std::make_unique<EntityPendingSpawnQueue>()),
		  _status_controller(std::make_unique<EntityStatusController>()),
		  _visual_state(std::make_unique<EntityVisualState>()) {}

	Entity::~Entity()
	{
		_audio_controller->stopMovementSound(_audio_manager);
		unloadModelData();
	}

	void Entity::setSharedResourceManager(Core::ResourceManager* manager)
	{
		g_shared_resource_manager = manager;
	}

	Core::ResourceManager* Entity::getSharedResourceManager()
	{
		return g_shared_resource_manager;
	}

	void Entity::setCombatEventBus(Game::CombatEventBus* event_bus)
	{
		g_combat_event_bus = event_bus;
	}

	Game::CombatEventBus* Entity::getCombatEventBus()
	{
		return g_combat_event_bus;
	}

	void Entity::assignEntityId(const EntityId entity_id)
	{
		if (_entity_id == INVALID_ENTITY_ID && entity_id != INVALID_ENTITY_ID)
			_entity_id = entity_id;
	}

	void Entity::beginCastTelemetry(std::string cast_name, const float duration_seconds, const bool interruptible)
	{
		_status_controller->beginCast(std::move(cast_name), duration_seconds, interruptible);
	}

	void Entity::clearCastTelemetry()
	{
		_status_controller->clearCast();
	}

	const EntityCastState& Entity::getCastState() const
	{
		return _status_controller->castState();
	}

	bool Entity::isCasting() const
	{
		return _status_controller->isCasting();
	}

	DamageSourceContext Entity::makeDamageSourceContext(Entity* source, std::string source_label)
	{
		DamageSourceContext context;
		if (!source)
			return context;

		context.valid = true;
		context.source = source->weak_from_this();
		context.source_id = source->getEntityId();
		context.source_name = source->getName();
		context.source_type = source->getType();
		context.source_faction = source->getFaction();
		context.source_position = source->getCenter();
		context.label = std::move(source_label);
		return context;
	}

	void Entity::setAudioListener(const std::shared_ptr<Entity>& listener)
	{
		EntityAudioController::setListener(listener);
	}

	void Entity::hideMeshIndex(const int mesh_index)
	{
		_visual_state->hideMeshIndex(mesh_index);
	}

	void Entity::setModelTint(const Color tint)
	{
		_visual_state->setModelTint(tint);
	}

	Color Entity::getModelTint() const
	{
		return _visual_state->modelTint();
	}

	void Entity::setHovered(const bool hovered)
	{
		_visual_state->setHovered(hovered);
	}

	void Entity::setModelFacingOffset(const float deg)
	{
		_visual_state->setModelFacingOffset(deg);
	}

	float Entity::getModelFacingOffset() const
	{
		return _visual_state->modelFacingOffset();
	}

	void Entity::update(const float delta_time)
	{
		if (_dormant)
			return;

		updateCastTelemetry(delta_time);
		updateStatusEffects(delta_time);

		if (_is_dying)
		{
			updateAnimation(delta_time);
			if (!isAnimationLocked())
				_hp = 0;
			return;
		}

		_pos.x += _velocity.x * delta_time;
		_pos.y += _velocity.y * delta_time;
		updateAnimation(delta_time);
	}

} // namespace Nawia::Entity
