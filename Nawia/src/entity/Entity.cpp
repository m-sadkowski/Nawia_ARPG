#include "Entity.h"

#include <Collider.h>
#include <EntityAbilityController.h>
#include <EntityAudioController.h>
#include <EntityModelState.h>
#include <EntityMovementState.h>
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
		: _model_state(std::make_unique<EntityModelState>()),
		  _movement_state(std::make_unique<EntityMovementState>()),
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
		: _model_state(std::make_unique<EntityModelState>()),
		  _movement_state(std::make_unique<EntityMovementState>()),
		  _name(name), _texture(texture), _max_hp(max_hp), _hp(max_hp),
		  _faction(Faction::None),
		  _ability_controller(std::make_unique<EntityAbilityController>()),
		  _audio_controller(std::make_unique<EntityAudioController>()),
		  _pending_spawn_queue(std::make_unique<EntityPendingSpawnQueue>()),
		  _status_controller(std::make_unique<EntityStatusController>()),
		  _visual_state(std::make_unique<EntityVisualState>()) {
		_movement_state->position = {start_x, start_y};
	}

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

	float Entity::getX() const
	{
		return _movement_state->position.x;
	}

	float Entity::getY() const
	{
		return _movement_state->position.y;
	}

	Vector2 Entity::getPosition() const
	{
		return _movement_state->position;
	}

	float Entity::getAltitude() const
	{
		return _movement_state->altitude;
	}

	void Entity::setX(const float x)
	{
		_movement_state->position.x = x;
	}

	void Entity::setY(const float y)
	{
		_movement_state->position.y = y;
	}

	void Entity::setPosition(const Vector2 position)
	{
		_movement_state->position = position;
	}

	void Entity::translatePosition(const float dx, const float dy)
	{
		_movement_state->position.x += dx;
		_movement_state->position.y += dy;
	}

	void Entity::setAltitude(const float altitude)
	{
		_movement_state->altitude = altitude;
	}

	Vector3 Entity::getWorldPos3D() const
	{
		return {_movement_state->position.x, _movement_state->altitude, _movement_state->position.y};
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

	void Entity::rememberDamageSource(Entity* source, std::string source_label)
	{
		_last_damage_source = makeDamageSourceContext(source, std::move(source_label));
	}

	void Entity::rememberDamageSource(DamageSourceContext source_context)
	{
		_last_damage_source = std::move(source_context);
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

		_movement_state->position.x += _movement_state->velocity.x * delta_time;
		_movement_state->position.y += _movement_state->velocity.y * delta_time;
		updateAnimation(delta_time);
	}

} // namespace Nawia::Entity
