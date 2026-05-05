#pragma once

#include <raylib.h>

#include <string>
#include <vector>

namespace Nawia::Core::System::Renderer {

	/**
	 * @struct Light
	 * @brief Dane pojedynczego swiatla oraz lokalizacje uniformow shadera.
	 */
	struct Light {
		int type = 0;
		int enabled = 1;
		Vector3 position = {};
		Vector3 target = {};
		Color color = WHITE;

		int enabled_uniform_location = -1;
		int type_uniform_location = -1;
		int position_uniform_location = -1;
		int target_uniform_location = -1;
		int color_uniform_location = -1;
	};

	/**
	 * @class LightingSystem
	 * @brief Zarzadza shaderem oswietlenia i lista swiatel sceny.
	 */
	class LightingSystem {
	public:
		enum LightType {
			LIGHT_DIRECTIONAL = 0,
			LIGHT_POINT = 1
		};

		LightingSystem();
		~LightingSystem();

		/**
		 * @brief Laduje shader i przygotowuje stale uniformy.
		 */
		void initialize();

		/**
		 * @brief Aktualizuje pozycje kamery w shaderze.
		 */
		void update(const Camera3D& camera);

		/**
		 * @brief Przypisuje shader do poprawnie zaladowanego modelu.
		 */
		void applyToModel(Model& model) const;

		/**
		 * @brief Dodaje swiatlo, jesli nie przekroczono limitu shadera.
		 */
		void addLight(int type, Vector3 position, Vector3 target, Color color);

		/**
		 * @brief Wysyla aktualne dane swiatla do shadera.
		 */
		void updateLightValues(int index);

		/**
		 * @brief Ustawia kolor swiatla ambient.
		 */
		void setAmbientColor(Color color);

		[[nodiscard]] Color getAmbientColor() const { return _ambient_color; }

		/**
		 * @brief Zapisuje ustawienia oswietlenia do JSON.
		 */
		void saveLightingToJson(const std::string& filepath) const;

		/**
		 * @brief Wczytuje ustawienia oswietlenia z JSON.
		 */
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
