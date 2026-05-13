#pragma once

#include <Level.h>

namespace Nawia::World {

	/**
	 * @class DemoLevel2
	 * @brief Demo oparte o lokacje zapisane Kreatorem leveli.
	 */
	class DemoLevel2 : public Level {
	public:
		/** @brief Wczytuje lokacje z plikow assets/data/locations. */
		void onEnter(Core::Engine* engine) override;

		/** @brief Zwraca nazwe poziomu demo z nowego systemu lokacji. */
		[[nodiscard]] std::string getName() const override { return "DemoLevel2"; }

		/** @brief Nowy system lokacji nie korzysta ze zbiorczego pliku spawnow. */
		[[nodiscard]] std::string getSpawnFilePath() const override { return ""; }

		/** @brief Zwraca lokacje dostepne w poziomie. */
		[[nodiscard]] std::vector<std::string> getLocations() const override {
			return {"Diabelski Las", "Lesna Dolina"};
		}

		/** @brief Przelacza lokacje przez loader JSON i dobiera muzyke. */
		void changeLocation(Core::Engine* engine, const std::string& location_name) override;

	private:
		void playLocationMusic(Core::Engine* engine, const std::string& location_name) const;
	};

} // namespace Nawia::World
