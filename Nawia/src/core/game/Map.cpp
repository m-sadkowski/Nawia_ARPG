#include "Map.h"

#include <Logger.h>

#include <raymath.h>

#include <filesystem>

namespace Nawia::Core {

	Map::Map(ResourceManager& resource_manager)
		: _resource_manager(resource_manager) {
	}

	Map::~Map() {
		if (_model_loaded)
			UnloadModel(_model);
	}

	void Map::loadMap(const std::string& filename, const float scale, const Vector3 offset, const Vector3 rotation) {
		if (_model_loaded) {
			UnloadModel(_model);
			_model_loaded = false;
		}

		const std::string filepath = "assets/maps/" + filename;
		_scale = scale;
		_offset = offset;
		_rotation = rotation;

		if (!std::filesystem::exists(filepath)) {
			Logger::errorLog("Map::loadMap - nie znaleziono pliku: " + filepath + ", ladowanie placeholdera");
			loadPlaceholder();
			return;
		}

		_model = LoadModel(filepath.c_str());

		if (_model.meshCount == 0) {
			Logger::errorLog("Map::loadMap - nie udalo sie zaladowac modelu: " + filepath + ", ladowanie placeholdera");
			loadPlaceholder();
			return;
		}

		_model_loaded = true;
		_is_placeholder = false;

		const Matrix rotation_x = MatrixRotateX(_rotation.x * DEG2RAD);
		const Matrix rotation_y = MatrixRotateY(_rotation.y * DEG2RAD);
		const Matrix rotation_z = MatrixRotateZ(_rotation.z * DEG2RAD);
		_model.transform = MatrixMultiply(MatrixMultiply(rotation_x, rotation_y), rotation_z);

		_navmesh.buildFromModel(_model, _scale, _offset);

		_mesh_bboxes.clear();
		_mesh_bboxes.reserve(_model.meshCount);
		for (int mesh_index = 0; mesh_index < _model.meshCount; ++mesh_index) {
			_mesh_bboxes.push_back(GetMeshBoundingBox(_model.meshes[mesh_index]));
		}

		Logger::debugLog("Map: zaladowano " + filename +
			" (meshes=" + std::to_string(_model.meshCount) +
			", scale=" + std::to_string(_scale) +
			", offset={" + std::to_string(_offset.x) + ", " + std::to_string(_offset.y) + ", " + std::to_string(_offset.z) +
			"}, rotation={" + std::to_string(_rotation.x) + ", " + std::to_string(_rotation.y) + ", " + std::to_string(_rotation.z) + "})");
	}

	void Map::loadPlaceholder() {
		const Mesh plane_mesh = GenMeshPlane(60.0f, 60.0f, 10, 10);
		_model = LoadModelFromMesh(plane_mesh);
		_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = Color{45, 55, 40, 255};

		_model_loaded = true;
		_is_placeholder = true;

		_mesh_bboxes.clear();
		_mesh_bboxes.push_back(GetMeshBoundingBox(_model.meshes[0]));

		Logger::debugLog("Map: zaladowano placeholder 60x60");
	}

	void Map::render() const {
		if (!_model_loaded) return;

		DrawModel(_model, _offset, _scale, WHITE);

		if (_is_placeholder)
			DrawGrid(60, 1.0f);
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

		Matrix model_transform = _model.transform;
		model_transform = MatrixMultiply(model_transform, MatrixScale(_scale, _scale, _scale));
		model_transform = MatrixMultiply(model_transform, MatrixTranslate(_offset.x, _offset.y, _offset.z));

		for (int mesh_index = 0; mesh_index < _model.meshCount; ++mesh_index) {
			const BoundingBox mesh_box = _mesh_bboxes[mesh_index];
			const Vector3 corners[8] = {
				{mesh_box.min.x, mesh_box.min.y, mesh_box.min.z}, {mesh_box.max.x, mesh_box.min.y, mesh_box.min.z},
				{mesh_box.min.x, mesh_box.max.y, mesh_box.min.z}, {mesh_box.max.x, mesh_box.max.y, mesh_box.min.z},
				{mesh_box.min.x, mesh_box.min.y, mesh_box.max.z}, {mesh_box.max.x, mesh_box.min.y, mesh_box.max.z},
				{mesh_box.min.x, mesh_box.max.y, mesh_box.max.z}, {mesh_box.max.x, mesh_box.max.y, mesh_box.max.z}
			};

			BoundingBox world_box = {{1e9f, 1e9f, 1e9f}, {-1e9f, -1e9f, -1e9f}};
			for (const Vector3& corner : corners) {
				const Vector3 transformed_corner = Vector3Transform(corner, model_transform);
				world_box.min = Vector3Min(world_box.min, transformed_corner);
				world_box.max = Vector3Max(world_box.max, transformed_corner);
			}

			if (!GetRayCollisionBox(ray, world_box).hit)
				continue;

			const RayCollision hit = GetRayCollisionMesh(ray, _model.meshes[mesh_index], model_transform);
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

		if (!_navmesh.buildFromModel(_model, _scale, _offset))
			Logger::errorLog("Map::setNavMeshMinWalkableHeight - nie udalo sie przebudowac navmesha.");
	}

} // namespace Nawia::Core
