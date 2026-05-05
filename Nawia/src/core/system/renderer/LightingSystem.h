#pragma once

#include <raylib.h>
#include <string>
#include <vector>

namespace Nawia::Core::System::Renderer {

	struct Light {
		int type;
		int enabled;
		Vector3 position;
		Vector3 target;
		Color color;
		
		int enabled_uniform_location;
		int type_uniform_location;
		int position_uniform_location;
		int target_uniform_location;
		int color_uniform_location;
	};

	class LightingSystem {
	public:
		enum LightType {
			LIGHT_DIRECTIONAL = 0,
			LIGHT_POINT = 1
		};

		LightingSystem();
		~LightingSystem();

		void initialize();
		void update(const Camera3D& camera);
		
		/** @brief Przypisuje shader do poprawnie załadowanego modelu. */
		void applyToModel(Model& model) const;
		
		void addLight(int type, Vector3 position, Vector3 target, Color color);
		void updateLightValues(int index);
		void setAmbientColor(Color color);
		[[nodiscard]] Color getAmbientColor() const { return _ambient_color; }
		
		void saveLightingToJson(const std::string& filepath) const;
		void loadLightingFromJson(const std::string& filepath);

		[[nodiscard]] std::vector<Light>& getLights() { return _lights; }
		[[nodiscard]] const std::vector<Light>& getLights() const { return _lights; }

	private:
		void cacheLightUniformLocations(Light& light, int light_index);
		void uploadLightToShader(const Light& light);

		Shader _shader = {};
		int _ambient_location = -1;
		Color _ambient_color = BLACK;
		int _view_position_location = -1;
		std::vector<Light> _lights;
		bool _is_initialized = false;
	};

} // namespace Nawia::Core::System::Renderer
