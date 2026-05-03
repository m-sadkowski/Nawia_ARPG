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
		
		int enabledLoc;
		int typeLoc;
		int positionLoc;
		int targetLoc;
		int colorLoc;
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
		
		// Assigns the shader to a model
		void applyToModel(Model& model) const;
		
		void addLight(int type, Vector3 position, Vector3 target, Color color);
		void updateLightValues(int index);
		void setAmbientColor(Color color);
		Color getAmbientColor() const { return _ambientColor; }
		
		void saveLightingToJson(const std::string& filepath) const;
		void loadLightingFromJson(const std::string& filepath);

		std::vector<Light>& getLights() { return _lights; }

	private:
		Shader _shader;
		int _ambientLoc;
		Color _ambientColor;
		int _viewPosLoc;
		std::vector<Light> _lights;
		int _lightCount;
		bool _isInitialized;
	};

} // namespace Nawia::Core::System::Renderer
