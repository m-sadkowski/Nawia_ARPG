#include "Map.h"

#include <Logger.h>

#include <raymath.h>

#include <cmath>
#include <filesystem>
#include <map>
#include <memory>
#include <utility>

namespace Nawia::Core {

	namespace {

		std::map<std::string, std::shared_ptr<Model>> g_map_model_cache;

		std::shared_ptr<Model> getCachedMapModel(const std::string& filename) {
			const auto cached_model = g_map_model_cache.find(filename);
			if (cached_model != g_map_model_cache.end())
				return cached_model->second;

			const std::string filepath = "assets/maps/" + filename;
			if (!std::filesystem::exists(filepath)) {
				Logger::errorLog("Map::loadMap - nie znaleziono pliku: " + filepath);
				return nullptr;
			}

			Model model = LoadModel(filepath.c_str());
			if (model.meshCount == 0) {
				Logger::errorLog("Map::loadMap - nie udalo sie zaladowac modelu: " + filepath);
				return nullptr;
			}

			auto loaded_model = std::shared_ptr<Model>(new Model(model), [](const Model* model_to_unload) {
				UnloadModel(*model_to_unload);
				delete model_to_unload;
			});
			g_map_model_cache[filename] = loaded_model;
			return loaded_model;
		}

		BoundingBox transformBoundingBox(const BoundingBox& box, const Matrix& transform) {
			const Vector3 corners[8] = {
				{box.min.x, box.min.y, box.min.z}, {box.max.x, box.min.y, box.min.z},
				{box.min.x, box.max.y, box.min.z}, {box.max.x, box.max.y, box.min.z},
				{box.min.x, box.min.y, box.max.z}, {box.max.x, box.min.y, box.max.z},
				{box.min.x, box.max.y, box.max.z}, {box.max.x, box.max.y, box.max.z}
			};

			BoundingBox world_box = {{1e9f, 1e9f, 1e9f}, {-1e9f, -1e9f, -1e9f}};
			for (const Vector3& corner : corners) {
				const Vector3 transformed_corner = Vector3Transform(corner, transform);
				world_box.min = Vector3Min(world_box.min, transformed_corner);
				world_box.max = Vector3Max(world_box.max, transformed_corner);
			}

			return world_box;
		}

		bool containsPoint(const BoundingBox& box, const Vector3& point) {
			return point.x >= box.min.x && point.x <= box.max.x &&
				   point.y >= box.min.y && point.y <= box.max.y &&
				   point.z >= box.min.z && point.z <= box.max.z;
		}

		bool isBoundingBoxVisible(const BoundingBox& box, const Camera3D& camera) {
			if (containsPoint(box, camera.position))
				return true;

			const int screen_width = GetScreenWidth();
			const int screen_height = GetScreenHeight();
			constexpr float screen_margin = 256.0f;

			const Vector3 corners[8] = {
				{box.min.x, box.min.y, box.min.z}, {box.max.x, box.min.y, box.min.z},
				{box.min.x, box.max.y, box.min.z}, {box.max.x, box.max.y, box.min.z},
				{box.min.x, box.min.y, box.max.z}, {box.max.x, box.min.y, box.max.z},
				{box.min.x, box.max.y, box.max.z}, {box.max.x, box.max.y, box.max.z}
			};

			float min_x = 1e9f;
			float min_y = 1e9f;
			float max_x = -1e9f;
			float max_y = -1e9f;

			for (const Vector3& corner : corners) {
				const Vector2 screen = GetWorldToScreenEx(corner, camera, screen_width, screen_height);
				if (!std::isfinite(screen.x) || !std::isfinite(screen.y))
					return true;

				min_x = std::min(min_x, screen.x);
				min_y = std::min(min_y, screen.y);
				max_x = std::max(max_x, screen.x);
				max_y = std::max(max_y, screen.y);
			}

			return max_x >= -screen_margin &&
				   min_x <= static_cast<float>(screen_width) + screen_margin &&
				   max_y >= -screen_margin &&
				   min_y <= static_cast<float>(screen_height) + screen_margin;
		}

	}

	Map::Map(ResourceManager& resource_manager)
		: _resource_manager(resource_manager) {
	}

	Map::~Map() {
		if (_model_loaded && _owns_model)
			UnloadModel(_model);
	}

	void Map::preloadMapModel(const std::string& filename) {
		if (!filename.empty() && filename != "placeholder")
			getCachedMapModel(filename);
	}

	void Map::clearPreloadedMapModels() {
		g_map_model_cache.clear();
	}

	void Map::loadMap(const std::string& filename, const float scale, const Vector3 offset, const Vector3 rotation) {
		if (_model_loaded && _owns_model) {
			UnloadModel(_model);
		}
		_model_loaded = false;
		_owns_model = false;

		_scale = scale;
		_offset = offset;
		_rotation = rotation;
		_navmesh_blockers.clear();

		const auto cached_model = getCachedMapModel(filename);
		if (!cached_model) {
			Logger::errorLog("Map::loadMap - ladowanie placeholdera zamiast: " + filename);
			loadPlaceholder();
			return;
		}

		_model = *cached_model;
		_model_loaded = true;
		_owns_model = false;
		_is_placeholder = false;

		const Matrix rotation_x = MatrixRotateX(_rotation.x * DEG2RAD);
		const Matrix rotation_y = MatrixRotateY(_rotation.y * DEG2RAD);
		const Matrix rotation_z = MatrixRotateZ(_rotation.z * DEG2RAD);
		_model.transform = MatrixMultiply(MatrixMultiply(rotation_x, rotation_y), rotation_z);

		_navmesh.buildFromModel(_model, _scale, _offset, _navmesh_blockers);

		_mesh_bboxes.clear();
		_world_mesh_bboxes.clear();
		_mesh_bboxes.reserve(_model.meshCount);
		_world_mesh_bboxes.reserve(_model.meshCount);

		_model_transform = _model.transform;
		_model_transform = MatrixMultiply(_model_transform, MatrixScale(_scale, _scale, _scale));
		_model_transform = MatrixMultiply(_model_transform, MatrixTranslate(_offset.x, _offset.y, _offset.z));

		for (int mesh_index = 0; mesh_index < _model.meshCount; ++mesh_index) {
			const BoundingBox local_box = GetMeshBoundingBox(_model.meshes[mesh_index]);
			_mesh_bboxes.push_back(local_box);
			_world_mesh_bboxes.push_back(transformBoundingBox(local_box, _model_transform));
		}

		Logger::debugLog("Map: zaladowano " + filename +
			" (meshes=" + std::to_string(_model.meshCount) +
			", scale=" + std::to_string(_scale) +
			", offset={" + std::to_string(_offset.x) + ", " + std::to_string(_offset.y) + ", " + std::to_string(_offset.z) +
			"}, rotation={" + std::to_string(_rotation.x) + ", " + std::to_string(_rotation.y) + ", " + std::to_string(_rotation.z) + "})");
	}

	void Map::loadPlaceholder() {
		if (_model_loaded && _owns_model) {
			UnloadModel(_model);
		}
		_model_loaded = false;
		_owns_model = false;

		_scale = 1.0f;
		_offset = {0.0f, 0.0f, 0.0f};
		_rotation = {0.0f, 0.0f, 0.0f};
		_navmesh_blockers.clear();

		const Mesh plane_mesh = GenMeshPlane(60.0f, 60.0f, 10, 10);
		_model = LoadModelFromMesh(plane_mesh);
		_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = Color{45, 55, 40, 255};

		_model_loaded = true;
		_owns_model = true;
		_is_placeholder = true;
		_model_transform = _model.transform;
		_navmesh.buildFromModel(_model, _scale, _offset, _navmesh_blockers);

		_mesh_bboxes.clear();
		_world_mesh_bboxes.clear();
		const BoundingBox local_box = GetMeshBoundingBox(_model.meshes[0]);
		_mesh_bboxes.push_back(local_box);
		_world_mesh_bboxes.push_back(transformBoundingBox(local_box, _model.transform));

		Logger::debugLog("Map: zaladowano placeholder 60x60");
	}

	void Map::render(const Camera3D& camera) const {
		if (!_model_loaded) return;

		if (_is_placeholder) {
			DrawModel(_model, _offset, _scale, WHITE);
			DrawGrid(60, 1.0f);
			return;
		}

		for (int mesh_index = 0; mesh_index < _model.meshCount; ++mesh_index) {
			if (mesh_index < static_cast<int>(_world_mesh_bboxes.size()) &&
				!isBoundingBoxVisible(_world_mesh_bboxes[mesh_index], camera))
				continue;

			int material_index = 0;
			if (_model.meshMaterial && mesh_index < _model.meshCount)
				material_index = _model.meshMaterial[mesh_index];

			if (material_index < 0 || material_index >= _model.materialCount)
				material_index = 0;

			DrawMesh(_model.meshes[mesh_index], _model.materials[material_index], _model_transform);
		}
	}

	RayCollision Map::getRayCollision(Ray ray) const {
		if (!_model_loaded) return {false, 0, {0, 0, 0}, {0, 0, 0}};

		if (_is_placeholder) {
			return GetRayCollisionQuad(
				ray,
				{-1000.0f, 0.0f, -1000.0f},
				{-1000.0f, 0.0f, 1000.0f},
				{1000.0f, 0.0f, 1000.0f},
				{1000.0f, 0.0f, -1000.0f}
			);
		}

		RayCollision closest_hit = {false, 1000000.0f, {0, 0, 0}, {0, 0, 0}};

		for (int mesh_index = 0; mesh_index < _model.meshCount; ++mesh_index) {
			const BoundingBox& world_box = _world_mesh_bboxes[mesh_index];

			if (!GetRayCollisionBox(ray, world_box).hit)
				continue;

			const RayCollision hit = GetRayCollisionMesh(ray, _model.meshes[mesh_index], _model_transform);
			if (hit.hit && hit.distance < closest_hit.distance)
				closest_hit = hit;
		}

		return closest_hit;
	}

	bool Map::isWalkable(float world_x, float world_z) const {
		if (!_navmesh.isReady()) return true;

		const Vector3 position = {world_x, 0.0f, world_z};
		const Vector3 closest = _navmesh.getClosestWalkablePosition(position);

		const float dx = closest.x - world_x;
		const float dz = closest.z - world_z;
		return (dx * dx + dz * dz) < 0.5f;
	}

	std::vector<Vector2> Map::findPath(Vector3 start, Vector3 end) const {
		if (!_navmesh.isReady()) return {{start.x, start.z}, {end.x, end.z}};
		return _navmesh.findPath(start, end);
	}

	void Map::setNavMeshMinWalkableHeight(const float height) {
		_navmesh.setMinWalkableHeight(height);

		if (!_model_loaded)
			return;

		if (!_navmesh.buildFromModel(_model, _scale, _offset, _navmesh_blockers))
			Logger::errorLog("Map::setNavMeshMinWalkableHeight - nie udalo sie przebudowac navmesha.");
	}

	void Map::setNavMeshBlockers(std::vector<World::NavMeshBlocker> blockers) {
		_navmesh_blockers = std::move(blockers);

		if (!_model_loaded)
			return;

		if (!_navmesh.buildFromModel(_model, _scale, _offset, _navmesh_blockers))
			Logger::errorLog("Map::setNavMeshBlockers - nie udalo sie przebudowac navmesha.");
	}

} // namespace Nawia::Core
