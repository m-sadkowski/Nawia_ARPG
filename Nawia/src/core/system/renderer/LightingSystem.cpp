#include "LightingSystem.h"
#include <raymath.h>
#include <json.hpp>
#include <fstream>
#include <filesystem>

namespace Nawia::Core::System::Renderer {

	LightingSystem::LightingSystem() : _lightCount(0), _isInitialized(false) {}

	LightingSystem::~LightingSystem() {
		if (_isInitialized) {
			UnloadShader(_shader);
		}
	}

	void LightingSystem::initialize() {
		_shader = LoadShader("../assets/shaders/lighting.vs", "../assets/shaders/lighting.fs");
		
		// Get some required shader locations
		_shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(_shader, "viewPos");
		
		// Ambient light level (some basic lighting)
		_ambientLoc = GetShaderLocation(_shader, "ambient");
		setAmbientColor(Color{25, 25, 25, 255}); // Default ambient

		_isInitialized = true;
	}

	void LightingSystem::addLight(int type, Vector3 position, Vector3 target, Color color) {
		if (_lightCount >= 4) return; // MAX_LIGHTS is 4

		Light light;
		light.enabled = 1;
		light.type = type;
		light.position = position;
		light.target = target;
		light.color = color;

		std::string lightName = "lights[" + std::to_string(_lightCount) + "]";

		light.enabledLoc = GetShaderLocation(_shader, (lightName + ".enabled").c_str());
		light.typeLoc = GetShaderLocation(_shader, (lightName + ".type").c_str());
		light.positionLoc = GetShaderLocation(_shader, (lightName + ".position").c_str());
		light.targetLoc = GetShaderLocation(_shader, (lightName + ".target").c_str());
		light.colorLoc = GetShaderLocation(_shader, (lightName + ".color").c_str());

		SetShaderValue(_shader, light.enabledLoc, &light.enabled, SHADER_UNIFORM_INT);
		SetShaderValue(_shader, light.typeLoc, &light.type, SHADER_UNIFORM_INT);

		// Send to shader with float arrays
		float pos[3] = { light.position.x, light.position.y, light.position.z };
		SetShaderValue(_shader, light.positionLoc, pos, SHADER_UNIFORM_VEC3);

		float tar[3] = { light.target.x, light.target.y, light.target.z };
		SetShaderValue(_shader, light.targetLoc, tar, SHADER_UNIFORM_VEC3);

		float col[4] = { (float)light.color.r / 255.0f, (float)light.color.g / 255.0f, 
						 (float)light.color.b / 255.0f, (float)light.color.a / 255.0f };
		SetShaderValue(_shader, light.colorLoc, col, SHADER_UNIFORM_VEC4);

		_lights.push_back(light);
		_lightCount++;
	}

	void LightingSystem::setAmbientColor(Color color) {
		_ambientColor = color;
		float ambient[4] = { (float)color.r / 255.0f, (float)color.g / 255.0f, 
							 (float)color.b / 255.0f, (float)color.a / 255.0f };
		SetShaderValue(_shader, _ambientLoc, ambient, SHADER_UNIFORM_VEC4);
	}

	void LightingSystem::updateLightValues(int index) {
		if (index < 0 || index >= _lights.size()) return;
		Light& light = _lights[index];
		SetShaderValue(_shader, light.enabledLoc, &light.enabled, SHADER_UNIFORM_INT);
		SetShaderValue(_shader, light.typeLoc, &light.type, SHADER_UNIFORM_INT);

		float pos[3] = { light.position.x, light.position.y, light.position.z };
		SetShaderValue(_shader, light.positionLoc, pos, SHADER_UNIFORM_VEC3);

		float tar[3] = { light.target.x, light.target.y, light.target.z };
		SetShaderValue(_shader, light.targetLoc, tar, SHADER_UNIFORM_VEC3);

		float col[4] = { (float)light.color.r / 255.0f, (float)light.color.g / 255.0f, 
						 (float)light.color.b / 255.0f, (float)light.color.a / 255.0f };
		SetShaderValue(_shader, light.colorLoc, col, SHADER_UNIFORM_VEC4);
	}

	void LightingSystem::saveLightingToJson(const std::string& filepath) const {
		nlohmann::json data;
		data["ambient"] = { _ambientColor.r, _ambientColor.g, _ambientColor.b, _ambientColor.a };
		nlohmann::json lights = nlohmann::json::array();
		for (const auto& light : _lights) {
			nlohmann::json l;
			l["type"] = light.type;
			l["enabled"] = light.enabled;
			l["position"] = { light.position.x, light.position.y, light.position.z };
			l["target"] = { light.target.x, light.target.y, light.target.z };
			l["color"] = { light.color.r, light.color.g, light.color.b, light.color.a };
			lights.push_back(l);
		}
		data["lights"] = lights;
		std::ofstream out(filepath);
		if (out.is_open()) {
			out << data.dump(4);
		}
	}

	void LightingSystem::loadLightingFromJson(const std::string& filepath) {
		if (!std::filesystem::exists(filepath)) return;
		std::ifstream file(filepath);
		if (!file.is_open()) return;
		nlohmann::json data;
		try {
			file >> data;
		} catch(...) { return; }

		if (data.contains("ambient")) {
			auto a = data["ambient"];
			setAmbientColor({ a[0], a[1], a[2], a[3] });
		}
		if (data.contains("lights")) {
			_lights.clear();
			_lightCount = 0;
			for (const auto& l : data["lights"]) {
				auto p = l["position"];
				auto t = l["target"];
				auto c = l["color"];
				addLight(l["type"], {p[0], p[1], p[2]}, {t[0], t[1], t[2]}, {c[0], c[1], c[2], c[3]});
			}
		}
	}

	void LightingSystem::update(const Camera3D& camera) {
		if (!_isInitialized) return;

		// Update view position
		float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
		SetShaderValue(_shader, _shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);
	}

	void LightingSystem::applyToModel(Model& model) const {
		if (!_isInitialized) return;

		for (int i = 0; i < model.materialCount; i++) {
			model.materials[i].shader = _shader;
		}
	}

} // namespace Nawia::Core::System::Renderer
