#pragma once

#include <LocationDefinition.h>
#include <SpawnManager.h>

#include <memory>
#include <string>
#include <vector>

namespace Nawia::Core {
    class Engine;
    class Map;
}

namespace Nawia::World {

	struct LevelLocationFile {
		std::string name;
		std::string path;
	};

    /**
     * @class Level
     * @brief Bazowa klasa grywalnego poziomu.
     *
     * Poziom posiada mape, liste logicznych lokacji i manager spawnow. Klasy
     * pochodne wskazuja pliki lokacji zapisane Kreatorem leveli.
     */
    class Level {
    public:
        virtual ~Level();

        /**
         * @brief Uruchamia poziom i przygotowuje jego zasoby.
         */
        virtual void onEnter(Core::Engine* engine) = 0;

        /**
         * @brief Sprzata poziom przy przejsciu na inny.
         */
        virtual void onExit(Core::Engine* engine);

        /**
         * @brief Obsluguje input specyficzny dla poziomu.
         */
        virtual void handleInput(Core::Engine* engine) {}

        /**
         * @brief Aktualizuje logike poziomu i aktywacje spawnow.
         */
        virtual void update(Core::Engine* engine, float dt);

        /**
         * @brief Rysuje dodatkowy interfejs poziomu.
         */
        virtual void renderUI(Core::Engine* engine) {}

        /**
         * @brief Zwraca mape poziomu albo nullptr, jesli poziom nie jest wczytany.
         */
        [[nodiscard]] Core::Map* getMap() const { return _map.get(); }

        /**
         * @brief Zwraca unikalna nazwe poziomu.
         */
        [[nodiscard]] virtual std::string getName() const = 0;

        /**
         * @brief Zwraca nazwy lokacji dostepnych w poziomie.
         */
        [[nodiscard]] virtual std::vector<std::string> getLocations() const;

        /**
         * @brief Zwraca, czy poziom uczestniczy w systemie zapisu i wczytywania.
         *
         * Standardowe poziomy fabularne zwracaja `true`; specjalne tryby (np.
         * kreator leveli) moga wylaczyc zapisy w calym swoim cyklu zycia.
         */
        [[nodiscard]] virtual bool allowsSaves() const { return true; }

        /**
         * @brief Zwraca manager spawnow poziomu.
         */
        [[nodiscard]] SpawnManager& getSpawnManager() { return _spawn_manager; }

        /**
         * @brief Zwraca nazwe aktualnej lokacji.
         */
        [[nodiscard]] std::string getCurrentLocationName() const;

        /**
         * @brief Przelacza aktywna lokacje i przenosi gracza na jej spawn.
         */
        virtual void changeLocation(Core::Engine* engine, const std::string& location_name);

        /**
         * @brief Odbudowuje runtime levelu po odrodzeniu gracza.
         */
        virtual void prepareForRespawn(Core::Engine* engine);

        /**
         * @brief Zwraca indeks aktualnej lokacji.
         */
        [[nodiscard]] size_t getCurrentLocationIndex() const { return _current_location_index; }

    protected:
        /**
         * @brief Wczytuje zestaw lokacji zapisanych przez kreator poziomow.
         */
        void loadLocations(
			Core::Engine* engine,
			const std::vector<LevelLocationFile>& location_files,
			const std::string& initial_location = ""
		);

        /**
         * @brief Przeladowuje aktywna lokacje z definicji kreatora.
         */
		bool loadLocationDefinition(
			Core::Engine* engine,
			size_t location_index,
			bool move_player_to_spawn,
			bool reload_entities = true
		);

        /**
         * @brief Wczytuje wszystkie encje lokacji jako dormant/aktywne bez przeladowania map.
         */
		void rebuildLocationEntityPool(Core::Engine* engine);

        /**
         * @brief Wstepnie laduje modele map z lokacji do cache.
         */
		void preloadLocationMapModels() const;

        std::unique_ptr<Core::Map> _map;
        size_t _current_location_index = 0;
        SpawnManager _spawn_manager;
		std::vector<LocationDefinition> _location_definitions;
		bool _uses_location_files = false;
    };

} // namespace Nawia::World
