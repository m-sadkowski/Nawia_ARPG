#pragma once

#include <SimpleMeleeEnemy.h>

#include <memory>
#include <vector>

namespace Nawia::Entity {

	/**
	 * @brief Zarazony mini-grzyb laczacy przeciwnika, kapsule robala i oczyszczony prop.
	 *
	 * Pierwsze obrazenia nie zabijaja grzyba: uwalniaja `Worm`, a model zostaje
	 * zamrozony jako skorupa. Po zabiciu robala encja przechodzi w oczyszczony
	 * wariant mini-grzyba, ktory moze byc prowadzony po trasie jako prop.
	 */
	class MiniMushroomInfected : public SimpleMeleeEnemy {
	public:
		/** @brief Uwalnia robala przy pierwszym trafieniu zamiast odejmowac HP. */
		void takeDamage(int dmg) override;
		/** @brief Aktualizuje tryb przeciwnika, zamrozonej skorupy albo oczyszczonego propa. */
		void update(float dt) override;
		/** @brief Wywolywane przez powiazanego robala po smierci; rozpoczyna oczyszczenie. */
		void purifyAfterWormDeath();
		[[nodiscard]] bool isPurified() const { return _purified; }
		/** @brief Ustawia pojedynczy cel ruchu dla oczyszczonego mini-grzyba. */
		void setPropDestination(Vector2 destination);
		/** @brief Ustawia wielopunktowa trase dla oczyszczonego mini-grzyba. */
		void setPropRoute(const std::vector<Vector2>& route);

	private:
		/** @brief Tworzy robala i laczy go zwrotnie z ta skorupa. */
		void spawnLinkedWorm();
		/** @brief Zatrzymuje animacje smierci na ostatniej klatce. */
		void freezeOnDeathFrame();
		/** @brief Obsluguje idle, skoki i ruch oczyszczonego propa. */
		void updatePurifiedProp(float dt);
		/** @brief Podmienia zarazony model na czysty zestaw animacji mini-grzyba. */
		void loadMiniMushroomAnimations();
		/** @brief Przechodzi do kolejnego punktu trasy propa, pomijajac juz osiagniete. */
		void moveToNextPropRoutePoint();

		MiniMushroomInfected();
		friend class MiniMushroomInfectedBuilder;

		bool _corruption_released = false;
		bool _corpse_frozen = false;
		bool _purified = false;
		bool _purifying = false;
		bool _jumping = false;
		bool _has_prop_destination = false;
		/** @brief Trasa uzywana, gdy mini-grzyb ma wrocic lub przejsc przez punkty fabularne. */
		std::vector<Vector2> _prop_route;
		size_t _prop_route_index = 0;
		Vector2 _prop_destination = {0.0f, 0.0f};
		float _jump_timer = 2.0f;
	};

	class MiniMushroomInfectedBuilder : public EnemyBuilder<MiniMushroomInfectedBuilder> {
	public:
		MiniMushroomInfectedBuilder() {
			_mushroom_ptr = std::unique_ptr<MiniMushroomInfected>(new MiniMushroomInfected());
			this->_entity = _mushroom_ptr.get();
		}

		std::unique_ptr<MiniMushroomInfected> build() {
			return std::move(_mushroom_ptr);
		}

	private:
		std::unique_ptr<MiniMushroomInfected> _mushroom_ptr;
	};

} // namespace Nawia::Entity
