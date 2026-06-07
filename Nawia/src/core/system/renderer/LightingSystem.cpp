#include "LightingSystem.h"

#include <raymath.h>

#include <json.hpp>

#include <array>
#include <filesystem>
#include <fstream>

namespace Nawia::Core::System::Renderer {

	namespace {

		using json = nlohmann::json;

		constexpr int k_max_lights = 4;
		constexpr Color k_default_ambient_color = Color{ 25, 25, 25, 255 };

		std::array<float, 3> toShaderVec3(const Vector3& vector) {
			return { vector.x, vector.y, vector.z };
		}

		std::array<float, 4> toShaderVec4(const Color& color) {
			return {
				static_cast<float>(color.r) / 255.0f,
				static_cast<float>(color.g) / 255.0f,
				static_cast<float>(color.b) / 255.0f,
				static_cast<float>(color.a) / 255.0f
			};
		}

	}

	LightingSystem::LightingSystem() = default;

	LightingSystem::~LightingSystem() {
		if (_is_initialized) {
			UnloadShader(_shader);
		}
	}

	void LightingSystem::initialize() {
		_shader = LoadShader("assets/shaders/lighting.vs", "assets/shaders/lighting.fs");
		
		_shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(_shader, "viewPos");
		_view_position_location = _shader.locs[SHADER_LOC_VECTOR_VIEW];
		
		_ambient_location = GetShaderLocation(_shader, "ambient");
		setAmbientColor(k_default_ambient_color);

		_is_initialized = true;
	}

	void LightingSystem::addLight(int type, Vector3 position, Vector3 target, Color color) {
		if (static_cast<int>(_lights.size()) >= k_max_lights)
			return;

		Light light = {};
		light.enabled = 1;
		light.type = type;
		light.position = position;
		light.target = target;
		light.color = color;

		cacheLightUniformLocations(light, static_cast<int>(_lights.size()));
		uploadLightToShader(light);

		_lights.push_back(light);
	}

	void LightingSystem::setAmbientColor(Color color) {
		_ambient_color = color;
		const auto ambient_color = toShaderVec4(color);
		SetShaderValue(_shader, _ambient_location, ambient_color.data(), SHADER_UNIFORM_VEC4);
	}

	void LightingSystem::updateLightValues(int index) {
		if (index < 0 || index >= static_cast<int>(_lights.size()))
			return;

		uploadLightToShader(_lights[static_cast<size_t>(index)]);
	}

	void LightingSystem::saveLightingToJson(const std::string& filepath) const {
		const std::filesystem::path output_path(filepath);
		if (!output_path.parent_path().empty())
			std::filesystem::create_directories(output_path.parent_path());

		json data;
		data["ambient"] = { _ambient_color.r, _ambient_color.g, _ambient_color.b, _ambient_color.a };
		json lights = json::array();
		for (const auto& light : _lights) {
			json l;
			l["type"] = light.type;
			l["enabled"] = light.enabled;
			l["position"] = { light.position.x, light.position.y, light.position.z };
			l["target"] = { light.target.x, light.target.y, light.target.z };
			l["color"] = { light.color.r, light.color.g, light.color.b, light.color.a };
			lights.push_back(l);
		}
		data["lights"] = lights;

		std::ofstream out(output_path);
		if (out.is_open()) {
			out << data.dump(4);
		}
	}

	void LightingSystem::loadLightingFromJson(const std::string& filepath) {
		if (!std::filesystem::exists(filepath)) return;

		std::ifstream file(filepath);
		if (!file.is_open())
			return;

		json data;
		try {
			file >> data;
		}
		catch (...) {
			return;
		}

		for (int i = 0; i < k_max_lights; ++i) {
			Light disabled_light = {};
			disabled_light.enabled = 0;
			disabled_light.type = LightingSystem::LIGHT_POINT;
			cacheLightUniformLocations(disabled_light, i);
			uploadLightToShader(disabled_light);
		}

		if (data.contains("ambient")) {
			const auto& a = data["ambient"];
			setAmbientColor({ a[0], a[1], a[2], a[3] });
		}

		if (data.contains("lights")) {
			_lights.clear();
			for (const auto& l : data["lights"]) {
				const auto& p = l["position"];
				const auto& t = l["target"];
				const auto& c = l["color"];
				addLight(l["type"], { p[0], p[1], p[2] }, { t[0], t[1], t[2] }, { c[0], c[1], c[2], c[3] });
			}
		}
	}

	void LightingSystem::update(const Camera3D& camera) {
		if (!_is_initialized)
			return;

		const auto camera_position = toShaderVec3(camera.position);
		SetShaderValue(_shader, _view_position_location, camera_position.data(), SHADER_UNIFORM_VEC3);
	}

	void LightingSystem::applyToModel(Model& model) const {
		if (!_is_initialized || model.materialCount <= 0 || model.materials == nullptr)
			return;

		if (model.materials[0].shader.id == _shader.id)
			return;

		for (int i = 0; i < model.materialCount; i++) {
			model.materials[i].shader = _shader;
		}
	}

	void LightingSystem::cacheLightUniformLocations(Light& light, int light_index) {
		const std::string light_name = "lights[" + std::to_string(light_index) + "]";

		light.enabled_uniform_location = GetShaderLocation(_shader, (light_name + ".enabled").c_str());
		light.type_uniform_location = GetShaderLocation(_shader, (light_name + ".type").c_str());
		light.position_uniform_location = GetShaderLocation(_shader, (light_name + ".position").c_str());
		light.target_uniform_location = GetShaderLocation(_shader, (light_name + ".target").c_str());
		light.color_uniform_location = GetShaderLocation(_shader, (light_name + ".color").c_str());
	}

	void LightingSystem::uploadLightToShader(const Light& light) {
		SetShaderValue(_shader, light.enabled_uniform_location, &light.enabled, SHADER_UNIFORM_INT);
		SetShaderValue(_shader, light.type_uniform_location, &light.type, SHADER_UNIFORM_INT);

		const auto light_position = toShaderVec3(light.position);
		SetShaderValue(_shader, light.position_uniform_location, light_position.data(), SHADER_UNIFORM_VEC3);

		const auto light_target = toShaderVec3(light.target);
		SetShaderValue(_shader, light.target_uniform_location, light_target.data(), SHADER_UNIFORM_VEC3);

		const auto light_color = toShaderVec4(light.color);
		SetShaderValue(_shader, light.color_uniform_location, light_color.data(), SHADER_UNIFORM_VEC4);
	}

} // namespace Nawia::Core::System::Renderer
