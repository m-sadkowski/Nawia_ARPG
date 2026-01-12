#pragma once

#include <memory>
#include <vector>
#include <raylib.h>

namespace Nawia::Entity {
    class Player;
    class Entity;
}

namespace Nawia::Core {
    class EntityManager;
    struct Camera;
}

namespace Nawia::UI {

    enum class MenuAction {
        None,
        Play,
        Exit
    };

    class UIHandler {
    public:
        UIHandler();
        ~UIHandler();

        void initialize(const std::shared_ptr<Entity::Player>& player, Core::EntityManager* entity_manager);
        
        void update(float dt);
        void render(const Core::Camera& camera) const;
        void renderMainMenu() const;
        
        MenuAction handleMenuInput();
        void handleInput();

    private:
        void renderPlayerHealthBar() const;
        void renderEnemyHealthBars(const Core::Camera& camera) const;
        
        void drawBar(float x, float y, float width, float height, float percentage, Color fg_color, Color bg_color) const;
        void drawMenuButton(const Rectangle& rect, const char* text, bool is_hovered) const;

        std::shared_ptr<Entity::Player> _player;
        Core::EntityManager* _entity_manager;
        Font _font;
    };

} // namespace Nawia::UI
