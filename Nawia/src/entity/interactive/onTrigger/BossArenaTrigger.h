#pragma once
#include <InteractiveTrigger.h>

#include <json.hpp>

#include <memory>
#include <string>

namespace Nawia::Core { class Engine; }
namespace Nawia::Game { struct BossData; }

namespace Nawia::Entity {

    /**
     * @class BossArenaTrigger
     * @brief Trigger startujacy walke z bossem po wejsciu gracza w obszar.
     *
     * Tworzony z poziomu JSON, identyfikator bossa mapuje sie na definicje
     * w `BossManager`. Nie restartuje juz aktywnej walki ani nie wznawia
     * pokonanego bossa.
     */
    class BossArenaTrigger : public InteractiveTrigger {
    public:
        /**
         * @brief Tworzy trigger areny bossa.
         * @param boss_id Identyfikator bossa z bosses.json.
         * @param x Pozycja srodka triggera na osi X.
         * @param y Pozycja srodka triggera na osi Y.
         * @param width Szerokosc obszaru triggera.
         * @param height Wysokosc obszaru triggera.
         */
        BossArenaTrigger(const std::string& boss_id, float x, float y, float width, float height);

        /** @brief Rozpoczyna walke z bossem, gdy gracz wejdzie w trigger. */
        void onTriggerEnter(Entity& other) override;

        /** @brief Aktualizuje bazowy stan encji. */
        void update(float dt) override;

        /** @brief Renderuje diagnostyczny kolider w trybie debug. */
        void render(const Camera3D& camera) override;

        /** @brief Trigger nie obsluguje bezposredniej interakcji, zwraca 0. */
        float getInteractionRange() override;
        [[nodiscard]] nlohmann::json serializeState() const override;
        void applyState(const nlohmann::json& state, Item::ItemDatabase* item_database = nullptr) override;

    private:
        [[nodiscard]] const Game::BossData* getBossData(Core::Engine* engine) const;
        [[nodiscard]] bool shouldRunIntro(Core::Engine* engine, const Game::BossData& boss_data) const;
        void showBossPreview(Core::Engine* engine, const Game::BossData& boss_data);
        void hideBossPreview();
        void openIntroDialogue(Core::Engine* engine, const Game::BossData& boss_data);
        void startBoss(Core::Engine* engine);

        std::string _boss_id;
        bool _intro_dialogue_open = false;
        bool _intro_completed = false;
        std::weak_ptr<Entity> _preview_boss;
    };

} // namespace Nawia::Entity
