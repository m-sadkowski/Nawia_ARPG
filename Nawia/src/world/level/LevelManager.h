#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Nawia::Core {
    class Engine;
}

namespace Nawia::World {

    class Level;

    /**
     * @struct LevelInfo
     * @brief Dane poziomu potrzebne warstwie UI.
     */
    struct LevelInfo {
        std::string name;
        std::vector<std::string> locations;
    };

    /**
     * @class LevelManager
     * @brief Rejestruje poziomy i obsluguje przelaczanie miedzy nimi.
     */
    class LevelManager {
    public:
        LevelManager() = default;
        ~LevelManager();

        /**
         * @brief Rejestruje poziom dostepny w grze.
         */
        void registerLevel(std::shared_ptr<Level> level);

        /**
         * @brief Przelacza gre na poziom o podanej nazwie.
         */
        void changeLevel(const std::string& name, Core::Engine* engine);

        /**
         * @brief Zwraca informacje o zarejestrowanych poziomach.
         */
        [[nodiscard]] std::vector<LevelInfo> getRegisteredLevelInfos() const;

        /**
         * @brief Przekazuje input do aktywnego poziomu.
         */
        void handleInput(Core::Engine* engine);

        /**
         * @brief Aktualizuje aktywny poziom.
         */
        void update(Core::Engine* engine, float dt);

        /**
         * @brief Rysuje UI aktywnego poziomu.
         */
        void renderUI(Core::Engine* engine);

        /**
         * @brief Zwraca aktywny poziom albo nullptr.
         */
        [[nodiscard]] Level* getCurrentLevel() const;

        /**
         * @brief Zwraca nazwe aktywnego poziomu.
         */
        [[nodiscard]] std::string getCurrentLevelName() const;

        /**
         * @brief Zwraca nazwe aktywnej lokacji.
         */
        [[nodiscard]] std::string getCurrentLocationName() const;

    private:
        std::unordered_map<std::string, std::shared_ptr<Level>> _levels;
        std::shared_ptr<Level> _current_level;
    };

} // namespace Nawia::World
