#include "EntityPathMotion.h"

#include <Entity.h>
#include <Map.h>

namespace Nawia::Entity::PathMotion {

	std::size_t buildPathToPoint(
		Entity& entity,
		Core::Map* map,
		const Vector2 target,
		std::vector<Vector2>& path)
	{
		path.clear();

		if (map && map->getNavMesh().isReady())
			path = map->findPath(entity.getWorldPos3D(), {target.x, entity.getAltitude(), target.y});

		const std::size_t nav_path_size = path.size();
		if (path.empty())
			path.push_back(target);

		trimPathStart(entity, path);

		if (!path.empty())
			entity.moveTo(path.front().x, path.front().y);
		else
			stopPathMovement(entity, path);

		return nav_path_size;
	}

	std::size_t buildPathToEntity(
		Entity& entity,
		Core::Map* map,
		const Entity& target,
		std::vector<Vector2>& path)
	{
		path.clear();

		if (map && map->getNavMesh().isReady())
			path = map->findPath(entity.getWorldPos3D(), target.getWorldPos3D());

		const std::size_t nav_path_size = path.size();
		if (path.empty())
			path.push_back(target.getCenter());

		trimPathStart(entity, path);

		if (!path.empty())
			entity.moveTo(path.front().x, path.front().y);
		else
			stopPathMovement(entity, path);

		return nav_path_size;
	}

	void trimPathStart(const Entity& entity, std::vector<Vector2>& path, const float reached_distance_sq)
	{
		if (path.empty())
			return;

		const Vector2 first_path_point = path.front();
		const float dx = first_path_point.x - entity.getCenter().x;
		const float dy = first_path_point.y - entity.getCenter().y;
		if (dx * dx + dy * dy < reached_distance_sq)
			path.erase(path.begin());
	}

	void updatePathMovement(Entity& entity, const float delta_time, std::vector<Vector2>& path)
	{
		if (!entity.isMoving() && !path.empty()) {
			path.erase(path.begin());

			if (!path.empty())
				entity.moveTo(path.front().x, path.front().y);
		}

		entity.updateMovement(delta_time);
	}

	void stopPathMovement(Entity& entity, std::vector<Vector2>& path)
	{
		path.clear();
		entity.stopMotion();
	}

} // namespace Nawia::Entity::PathMotion
