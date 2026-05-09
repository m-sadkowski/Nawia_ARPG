#pragma once
#include <InteractiveTrigger.h>

#include <string>

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

    private:
        std::string _boss_id;
    };

} // namespace Nawia::Entity
