#pragma once
#include <InteractiveTrigger.h>

#include <string>

namespace Nawia::Core { class Engine; }

namespace Nawia::Entity {

    /**
     * @class Teleport
     * @brief Trigger przenoszący gracza do innej lokacji w obrębie poziomu.
     */
    class Teleport : public InteractiveTrigger {
    public:
        Teleport(const std::string& name, float x, float y, Core::Engine* engine, const std::string& target_location);

        void onTriggerEnter(Entity& other) override;
        void update(float delta_time) override;
        void render(const Camera3D& camera) override;

        float getInteractionRange() override;

    private:
        Core::Engine* _engine;
        std::string _target_location;
        bool _snapped_to_navmesh = false;
    };

} // namespace Nawia::Entity
