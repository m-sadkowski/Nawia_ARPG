#include "BossArenaTrigger.h"

#include <Collider.h>
#include <Dialogue.h>
#include <Engine.h>
#include <BossManager.h>
#include <Frog.h>
#include <Player.h>
#include <QuestManager.h>

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
                    auto* engine = player->getEngine();
                    auto& boss_mgr = engine->getBossManager();
                    if (boss_mgr.isFightActive() || _intro_dialogue_open) return;

                    if (shouldRunRopuchIntro(engine)) {
                        showBossPreview(engine);
                        openRopuchIntro(engine);
                        return;
                    }

                    startBoss(engine);
                }
            }
        }
    }

    bool BossArenaTrigger::shouldRunRopuchIntro(Core::Engine* engine) const {
        if (!engine || _boss_id != "ropuch" || _intro_completed)
            return false;

        const auto* find_quest = engine->getQuestManager().getQuest("find_ropuch");
        const auto* scarf_quest = engine->getQuestManager().getQuest("recover_scarf");
        return find_quest && find_quest->isActive() && (!scarf_quest || !scarf_quest->isActive());
    }

    void BossArenaTrigger::showBossPreview(Core::Engine* engine) {
        if (!engine || _boss_id != "ropuch")
            return;

        if (const auto existing = _preview_boss.lock()) {
            existing->setDormant(false);
            return;
        }

        auto frog = std::make_shared<Frog>();
        frog->setName("Ropuch");
        frog->setType(EntityType::NPCStatic);
        frog->setFaction(Faction::None);
        frog->setTarget(nullptr);
        frog->setScale(1.5f);
        frog->setX(getCenter().x);
        frog->setY(getCenter().y);
        frog->setAltitude(getAltitude());
        frog->setMap(engine->getCurrentMap());
        frog->setAudioManager(&engine->getAudioManager());
        _preview_boss = frog;
        engine->spawnEntity(frog);
    }

    void BossArenaTrigger::hideBossPreview() {
        if (const auto preview = _preview_boss.lock())
            preview->setDormant(true);
    }

    void BossArenaTrigger::openRopuchIntro(Core::Engine* engine) {
        if (!engine)
            return;

        Game::DialogueTree tree;
        Game::DialogueNode node;
        node.id = 0;
        node.speaker_name = "Jarko";
        node.text = "Alez obrzydlistwo. CO ON MA W GNIEZDZIE? TO JEJ CHUSTA. Musze sie z nim rozprawic.";

        Game::DialogueOption option;
        option.text = "Rozprawie sie z nim.";
        option.next_node_id = -1;
        node.options.push_back(option);
        tree.addNode(node);

        _intro_dialogue_open = true;
        engine->getUIHandler().openDialogue(tree, 0, [this, engine](const int, const bool completed) {
            _intro_dialogue_open = false;
            if (!completed)
                return;

            _intro_completed = true;
            engine->getQuestManager().notifyCheckpointReached("Ropuch Trigger");
            engine->getQuestManager().update(engine);
            startBoss(engine);
        });
    }

    void BossArenaTrigger::startBoss(Core::Engine* engine) {
        if (!engine)
            return;

        hideBossPreview();
        engine->getBossManager().startBossFight(_boss_id, engine, getCenter(), getAltitude());
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
