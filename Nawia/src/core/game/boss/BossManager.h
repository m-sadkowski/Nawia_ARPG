#pragma once

#include <BossTypes.h>
#include <json.hpp>

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>

namespace Nawia::Entity {
    class EnemyInterface;
    class Entity;
}

namespace Nawia::Core {
    class Engine;
}

namespace Nawia::Game {

    /**
     * @class BossManager
     * @brief Zarzadza definicjami bossow, cyklem walki, fazami i nagrodami.
     *
     * Laduje dane bossow z JSON, preloaduje encje wskazane przez boss trigger,
     * startuje i konczy walke, obsluguje przejscia miedzy fazami oraz
     * spawnowanie minionow.
     */
    class BossManager {
    public:
        BossManager();
        ~BossManager();

        /**
         * @brief Laduje definicje bossow z pliku JSON.
         * @param path Sciezka do pliku, np. "assets/data/bosses.json".
         */
        void loadFromJson(const std::string& path);

        /**
         * @brief Preloaduje konkretna walke z bossem, np. po wczytaniu boss triggera z lokacji.
         */
        void preloadBossFight(const std::string& boss_id, Core::Engine* engine);

        /**
         * @brief Czyści preloadowane pule bossow i minionow.
         */
        void clearPreloadedBosses();
        void resetRuntimeState(Core::Engine* engine);

        /**
         * @brief Aktualizuje stan walki: timer, smierc bossa, fazy.
         * @param engine Wskaznik na silnik.
         * @param dt Czas od poprzedniej klatki w sekundach.
         */
        void update(Core::Engine* engine, float dt);
        
        /**
         * @brief Rozpoczyna walke z bossem o podanym ID.
         * @param boss_id Identyfikator bossa z JSON.
         * @param engine Wskaznik na silnik.
         * @return true jesli walka zostala rozpoczeta.
         */
        bool startBossFight(const std::string& boss_id, Core::Engine* engine);

        /**
         * @brief Rozpoczyna walke z bossem w konkretnym punkcie swiata.
         */
        bool startBossFight(
            const std::string& boss_id,
            Core::Engine* engine,
            Vector2 spawn_pos,
            float spawn_altitude
        );

        /**
         * @brief Konczy aktywna walke z bossem.
         * @param victory true jesli gracz wygral, false jesli przegral.
         * @param engine Wskaznik na silnik.
         */
        void endBossFight(bool victory, Core::Engine* engine);
        bool retryActiveBossFight(Core::Engine* engine);

        [[nodiscard]] bool isFightActive() const { return _active_boss_data != nullptr; }
        [[nodiscard]] const BossData* getActiveBossData() const { return _active_boss_data; }
        [[nodiscard]] std::shared_ptr<Entity::EnemyInterface> getActiveBossEntity() const { return _active_boss_entity; }
        [[nodiscard]] int getCurrentPhaseIndex() const { return _current_phase_index; }
        [[nodiscard]] float getFightTimer() const { return _fight_timer; }
        [[nodiscard]] bool isBossDefeated(const std::string& boss_id) const { return _defeated_bosses.count(boss_id) > 0; }
        [[nodiscard]] std::vector<std::string> getDefeatedBossIds() const;
        void setDefeatedBossIds(const std::vector<std::string>& boss_ids);
        void clearDefeatedBosses() { _defeated_bosses.clear(); }
        [[nodiscard]] BossRuntimeState getRuntimeState() const;
        bool restoreRuntimeState(const BossRuntimeState& state, Core::Engine* engine);

        /** @brief Serializuje aktywna walke z bossem do JSON-a (pusty obiekt, gdy brak walki). */
        [[nodiscard]] nlohmann::json serializeRuntimeState() const;

        /** @brief Odtwarza aktywna walke z bossem z JSON-a. Zwraca false, gdy stan jest pusty/nieaktywny. */
        bool applyRuntimeState(const nlohmann::json& state, Core::Engine* engine);

        [[nodiscard]] const std::map<std::string, BossData>& getAllBosses() const { return _bosses; }
        [[nodiscard]] std::shared_ptr<Entity::Entity> createPreviewEntity(const BossData& boss_data, Core::Engine* engine);

        /** @brief Zwraca pozostaly czas efektu blysku fazy, 0 gdy nieaktywny. */
        [[nodiscard]] float getPhaseFlashTimer() const { return _phase_flash_timer; }

        /** @brief Zwraca kolor blysku aktualnej fazy. */
        [[nodiscard]] Color getPhaseFlashColor() const { return _phase_flash_color; }

    private:
        std::map<std::string, BossData> _bosses;
        
        const BossData* _active_boss_data = nullptr;
        std::shared_ptr<Entity::EnemyInterface> _active_boss_entity = nullptr;
        
        // System faz.
        int _current_phase_index = 0;
        float _fight_timer = 0.0f;
        
        // Efekt blysku przy przejsciu fazy.
        float _phase_flash_timer = 0.0f;
        Color _phase_flash_color = { 255, 0, 0, 180 };
        
        void checkPhaseTransition(Core::Engine* engine);
        void applyPhase(const BossPhase& phase, Core::Engine* engine);
        
        // Miniony.
        void spawnMinions(const std::vector<MinionSpawnInfo>& minions, Core::Engine* engine);
        void removeMinions(Core::Engine* engine);
        std::vector<std::shared_ptr<Entity::Entity>> _active_minions;
        std::map<std::string, std::vector<std::shared_ptr<Entity::Entity>>> _minion_pools;
        std::map<std::string, std::shared_ptr<Entity::Entity>> _boss_pool;
        
        // Sledzenie pokonanych bossow, zeby walka nie mogla byc powtorzona.
        std::set<std::string> _defeated_bosses;

        // Helpery startowania walki.
        bool activateBossFromPool(const std::string& boss_id, Core::Engine* engine);
        bool buildAndActivateBoss(Core::Engine* engine);
        bool startBossFightAt(
            const std::string& boss_id,
            Core::Engine* engine,
            bool use_spawn_override,
            Vector2 spawn_pos,
            float spawn_altitude
        );
        [[nodiscard]] Vector3 resolveBossSpawnPosition(Core::Engine* engine) const;
        void placeEntityAtBossSpawn(const std::shared_ptr<Entity::Entity>& entity, Core::Engine* engine) const;
        void startBossMusic(Core::Engine* engine);
        void restoreMusicAfterBoss(Core::Engine* engine);

        Vector2 _active_boss_spawn_pos = {0.0f, 0.0f};
        float _active_boss_spawn_altitude = 0.0f;
        bool _boss_music_overrode_track = false;
        bool _had_music_before_boss = false;
        std::string _music_before_boss_path;
        float _music_before_boss_volume = 1.0f;
    };

} // namespace Nawia::Game
