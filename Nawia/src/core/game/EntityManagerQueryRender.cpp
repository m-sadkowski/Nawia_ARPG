#include "EntityManager.h"

#include <Entity.h>

#include <algorithm>

namespace Nawia::Core {

    std::shared_ptr<Entity::Entity> EntityManager::getEntityAt(
        const float screen_x,
        const float screen_y,
        const Camera3D& camera) const
    {
        for (auto it = _active_entities.rbegin(); it != _active_entities.rend(); ++it) {
            if ((*it)->isDormant())
                continue;

            if ((*it)->isMouseOver(screen_x, screen_y, camera))
                return *it;
        }

        return nullptr;
    }

    void EntityManager::updateHoverState(const float screen_x, const float screen_y, const Camera3D& camera) {
        if (const auto previous_hovered = _hovered_entity.lock())
            previous_hovered->setHovered(false);

        _hovered_entity.reset();

        for (auto it = _active_entities.rbegin(); it != _active_entities.rend(); ++it) {
            if ((*it)->isDormant())
                continue;

            if ((*it)->isMouseOver(screen_x, screen_y, camera)) {
                (*it)->setHovered(true);
                _hovered_entity = *it;
                return;
            }
        }
    }

    void EntityManager::renderEntities(const Camera3D& camera) const {
        std::vector<Entity::Entity*> render_list;
        render_list.reserve(_active_entities.size());

        for (const auto& entity : _active_entities) {
            if (!entity->isDormant() && entity->isVisibleInCamera(camera))
                render_list.push_back(entity.get());
        }

        std::ranges::sort(render_list, {}, &Entity::Entity::getY);

        for (auto* entity : render_list)
            entity->render(camera);
    }

} // namespace Nawia::Core
