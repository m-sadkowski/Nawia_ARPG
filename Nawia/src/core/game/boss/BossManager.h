#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <raylib.h>

namespace Nawia::Entity {
    class EnemyInterface;
    class Entity;
}

namespace Nawia::Core {
    class Engine;
}

namespace Nawia::Game {

    /**
     * @brief Nagroda przyznawana graczowi po pokonaniu bossa.
     */
    struct BossReward {
        std::vector<int> item_ids;
        int gold = 0;
        int exp = 0;
    };

    /**
     * @brief Opis pojedynczego typu miniona przywolywanego w fazie bossa.
     */
    struct MinionSpawnInfo {
        std::string enemy_type;   ///< Typ wroga, np. "WalkingDead", "Bandit", "Devil".
        int count = 1;
        int hp = 60;
        float offset_x = 3.0f;   ///< Przesuniecie spawnu wzgledem bossa na osi X.
        float offset_y = 2.0f;   ///< Przesuniecie spawnu wzgledem bossa na osi Y.
    };

    /**
     * @brief Definicja pojedynczej fazy walki z bossem.
     *
     * Faza aktywuje sie, gdy procent HP bossa spadnie ponizej `hp_threshold`.
     */
    struct BossPhase {
        float hp_threshold = 1.0f;   ///< Prog aktywacji fazy (1.0 = 100% HP).
        std::string name;
        float speed_multiplier = 1.0f;
        float damage_multiplier = 1.0f;
        std::string notification;
        std::vector<MinionSpawnInfo> minions;
        bool screen_flash = false;          ///< Czy ekran ma blysnac przy wejsciu w faze.
        Color flash_color = { 255, 0, 0, 180 }; ///< Kolor efektu blysku.
    };

    /**
     * @brief Pelna definicja bossa ladowana z JSON.
     */
    struct BossData {
        std::string id;
        std::string name;
        std::string enemy_type; ///< Typ wroga, np. "Devil".
        int max_hp = 1000;
        float scale = 1.0f;
        
        Vector2 spawn_pos = { 0.0f, 0.0f };
        
        std::vector<BossPhase> phases;
        BossReward reward;
        std::string level_name;
        
        /// Strategia po smierci gracza: "end_fight" = koniec walki, "retry" = mozliwosc ponowienia.
        std::string on_player_death = "end_fight";
    };

    /**
     * @class BossManager
     * @brief Zarzadza definicjami bossow, cyklem walki, fazami i nagrodami.
     *
     * Laduje dane bossow z JSON, preloaduje encje dla aktywnego poziomu,
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
         * @brief Preladowuje encje bossa i minionow dla podanego poziomu.
         * @param level_name Nazwa poziomu dopasowana do `Level::getName()`.
         * @param engine Wskaznik na silnik do tworzenia encji.
         */
        void preloadForLevel(const std::string& level_name, Core::Engine* engine);

        /**
         * @brief Preloaduje konkretna walke z bossem, np. po wczytaniu boss triggera z lokacji.
         */
        void preloadBossFight(const std::string& boss_id, Core::Engine* engine);

        /**
         * @brief Czyści preloadowane pule bossow i minionow.
         */
        void clearPreloadedBosses();

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

        [[nodiscard]] bool isFightActive() const { return _active_boss_data != nullptr; }
        [[nodiscard]] const BossData* getActiveBossData() const { return _active_boss_data; }
        [[nodiscard]] std::shared_ptr<Entity::EnemyInterface> getActiveBossEntity() const { return _active_boss_entity; }
        [[nodiscard]] int getCurrentPhaseIndex() const { return _current_phase_index; }
        [[nodiscard]] float getFightTimer() const { return _fight_timer; }
        [[nodiscard]] bool isBossDefeated(const std::string& boss_id) const { return _defeated_bosses.count(boss_id) > 0; }

        [[nodiscard]] const std::map<std::string, BossData>& getAllBosses() const { return _bosses; }

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

        // Helpery preloadowania.
        std::shared_ptr<Entity::Entity> buildEnemyEntity(const std::string& type, const std::string& name,
                                                          int max_hp, Core::Engine* engine);
        bool preloadBossDefinition(const BossData& boss, Core::Engine* engine);

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

        Vector2 _active_boss_spawn_pos = {0.0f, 0.0f};
        float _active_boss_spawn_altitude = 0.0f;
    };

} // namespace Nawia::Game
