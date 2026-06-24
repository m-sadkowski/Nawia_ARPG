#pragma once

#include <raylib.h>

#include <memory>

namespace Nawia::Entity { class Player; }
namespace Nawia::Core { class ResourceManager; }

namespace Nawia::UI {

    /**
     * @class StatsUI
     * @brief Rysuje panel statystyk aktualnego gracza.
     */
    class StatsUI {
    public:
        StatsUI(const std::shared_ptr<Entity::Player>& player);

        /** @brief Laduje tlo panelu statystyk. */
        void loadResources(Core::ResourceManager& resource_manager);
        void setPlayer(const std::shared_ptr<Entity::Player>& player) { _player = player; }

        /** @brief Rysuje panel statystyk gracza. */
        void render(float x, float y, const Font& font) const;

    private:
        std::shared_ptr<Entity::Player> _player;
        std::shared_ptr<Texture2D> _background;
    };

} // namespace Nawia::UI
