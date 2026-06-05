#pragma once

#include <EnemyInterface.h>

#include <string>
#include <vector>

namespace Nawia::Entity {

	/**
	 * @brief Bazowe AI dla przeciwnikow walczacych prostym atakiem w zwarciu.
	 *
	 * To nie jest `Ability`: klasa laczy stan przeciwnika, pogon po navmeshu,
	 * okno zadania obrazen w animacji oraz reakcje na trafienie. Ewentualny
	 * przyszly refactor powinien wydzielic sam moment zadawania obrazen do
	 * ability, ale sterowanie Idle/Chasing/Attacking zostaje odpowiedzialnoscia AI.
	 */
	class SimpleMeleeEnemy : public EnemyInterface {
	public:
		/** @brief Aktualizuje maszyne stanow przeciwnika. */
		void update(float dt) override;
		/** @brief Odtwarza reakcje na trafienie, jesli konkretny przeciwnik ma taka animacje. */
		void takeDamage(int dmg) override;

	protected:
		SimpleMeleeEnemy();

		/** @brief Minimalny zestaw stanow potrzebny do AI walki w zwarciu. */
		enum class State {
			Idle,
			Chasing,
			Attacking,
			GettingHit
		};

		void configureCombat(
			float vision_range,
			float attack_range,
			float movement_speed,
			int attack_damage,
			float attack_cooldown,
			float attack_animation_speed = 1.0f,
			float attack_damage_frame_ratio = 0.45f
		);

		/** @brief Mapuje nazwy animacji konkretnego modelu na stany bazowego AI. */
		void configureAnimations(
			const std::string& idle_animation,
			const std::string& walk_animation,
			const std::string& attack_animation,
			const std::string& hit_animation = ""
		);
		/** @brief Punkt rozszerzenia dla klas pochodnych po faktycznym zadaniu obrazen. */
		virtual void onAttackDamageApplied(Entity& target) {}
		/** @brief Idzie po navmeshu do punktu, przeliczajac trase co `repath_interval`. */
		bool moveTowardPositionWithNav(Vector2 target_pos, float dt, float repath_interval = 0.35f);
		/** @brief Sprawdza, czy cel ma sciezke po navmeshu zanim rozpoczniemy atak. */
		[[nodiscard]] bool canReachPositionWithNav(Vector2 target_pos) const;
		/** @brief Czyści cache sciezki i wymusza przeliczenie przy nastepnym ruchu. */
		void clearNavigationPath();

		State _state = State::Idle;
		State _state_before_hit = State::Idle;
		float _vision_range = 8.0f;
		float _attack_range = 1.2f;
		float _base_movement_speed = 2.0f;
		int _attack_damage = 15;
		float _attack_cooldown = 1.2f;
		float _attack_animation_speed = 1.0f;
		/** @brief Procent czasu animacji, w ktorym trafienie ma zadac obrazenia. */
		float _attack_damage_frame_ratio = 0.45f;
		float _attack_cooldown_timer = 0.0f;
		bool _attack_damage_applied = false;

		std::string _idle_animation = "idle";
		std::string _walk_animation = "walk";
		std::string _attack_animation = "attack";
		std::string _hit_animation;
		/** @brief Ostatnio wyliczona sciezka do celu, trzymana dla ograniczenia kosztu navmesha. */
		std::vector<Vector2> _current_nav_path;
		Vector2 _current_nav_target = {0.0f, 0.0f};
		bool _has_current_nav_target = false;

	private:
		void handleIdleState(float dt);
		void handleChasingState(float dt);
		void handleAttackingState(float dt);
		void handleGettingHitState(float dt);
	};

} // namespace Nawia::Entity
