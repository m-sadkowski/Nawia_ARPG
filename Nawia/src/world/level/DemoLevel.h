#pragma once

#include <Level.h>

namespace Nawia::World {

	/**
	 * @class DemoLevel
	 * @brief Demo oparte o lokacje zapisane Kreatorem leveli.
	 */
	class DemoLevel : public Level {
	public:
		/** @brief Wczytuje lokacje z plikow assets/data/locations. */
		void onEnter(Core::Engine* engine) override;
		void onNewGameStarted(Core::Engine* engine) override;

		/** @brief Zwraca nazwe poziomu demo. */
		[[nodiscard]] std::string getName() const override { return "Demo"; }

        /** @brief Zwraca lokacje dostepne w poziomie demo. */
        [[nodiscard]] std::vector<std::string> getLocations() const override {
            return {"Diabelski Las", "Lesna Dolina"};
        }

        [[nodiscard]] std::vector<LevelLocationFile> getLocationFiles() const override;
        [[nodiscard]] std::string getDefaultInitialLocation() const override { return "Diabelski Las"; }

		/** @brief Przelacza lokacje przez loader JSON i dobiera muzyke. */
		void changeLocation(Core::Engine* engine, const std::string& location_name) override;

	private:
		void playLocationMusic(Core::Engine* engine, const std::string& location_name) const;
	};

} // namespace Nawia::World
