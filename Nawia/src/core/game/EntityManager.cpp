#include "EntityManager.h"

#include <Entity.h>

#include <algorithm>
#include <utility>

namespace Nawia::Core {

    void EntityManager::addEntity(std::shared_ptr<Entity::Entity> new_entity) {
        if (!new_entity)
            return;

        if (std::ranges::find(_active_entities, new_entity) != _active_entities.end())
            return;

        assignEntityIdIfMissing(new_entity);
        _active_entities.push_back(std::move(new_entity));
    }

    void EntityManager::removeEntity(const std::shared_ptr<Entity::Entity>& entity) {
        if (!entity)
            return;

        if (_hovered_entity.lock() == entity) {
            entity->setHovered(false);
            _hovered_entity.reset();
        }

        _active_entities.erase(
            std::remove(_active_entities.begin(), _active_entities.end(), entity),
            _active_entities.end());
    }

    void EntityManager::setPlayer(std::shared_ptr<Entity::Entity> player) {
        _player = std::move(player);
        assignEntityIdIfMissing(_player);
        Entity::Entity::setAudioListener(_player);
    }

    void EntityManager::assignEntityIdIfMissing(const std::shared_ptr<Entity::Entity>& entity) {
        if (!entity || entity->hasEntityId())
            return;

        entity->assignEntityId(_next_entity_id++);
    }

	void EntityManager::clearNonPlayerEntities() {
		std::vector<std::shared_ptr<Entity::Entity>> retained_entities;
		if (_player) {
			retained_entities.push_back(_player);
		} else {
			for (const auto& entity : _active_entities) {
				if (entity->getName() == "Player") {
					retained_entities.push_back(entity);
					break;
				}
			}
		}

		_active_entities = std::move(retained_entities);
        _hovered_entity.reset();
        _combat_target_refresh_timer = 0.0f;
        _altitude_snap_timer = 0.0f;
	}

} // namespace Nawia::Core
