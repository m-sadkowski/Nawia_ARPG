#include "BossArenaTrigger.h"

#include <Collider.h>
#include <Engine.h>
#include <BossManager.h>
#include <Player.h>

namespace Nawia::Entity {

    BossArenaTrigger::BossArenaTrigger(const std::string& boss_id, float x, float y, float width, float height)
        : InteractiveTrigger("BossTrigger_" + boss_id, x, y, nullptr, 1), _boss_id(boss_id)
    {
        setType(EntityType::Trigger);
        setFaction(Faction::None);
        setCollider(std::make_unique<RectangleCollider>(this, width, height, 0.0f, 0.0f));
    }

    void BossArenaTrigger::onTriggerEnter(Entity& other) {
        if (other.getFaction() == Faction::Player) {
            if (auto* player = dynamic_cast<Player*>(&other)) {
                if (player->getEngine()) {
                    auto& boss_mgr = player->getEngine()->getBossManager();
                    if (boss_mgr.isFightActive()) return;
                    boss_mgr.startBossFight(_boss_id, player->getEngine(), getCenter(), player->getAltitude());
                }
            }
        }
    }

    void BossArenaTrigger::update(float dt) {
        Entity::update(dt);
    }

    void BossArenaTrigger::render(const Camera3D& camera) {
        if (DebugColliders && _collider) {
            auto* rect_collider = dynamic_cast<RectangleCollider*>(_collider.get());
            if (rect_collider) {
                Vector2 center = rect_collider->getPosition();
                float w = rect_collider->getWidth();
                float h = rect_collider->getHeight();
                DrawCubeWires(Vector3{center.x, getAltitude() + 0.1f, center.y}, w, 0.2f, h, PURPLE);
            }
        }
    }

    float BossArenaTrigger::getInteractionRange() {
        return 0.0f;
    }

} // namespace Nawia::Entity
