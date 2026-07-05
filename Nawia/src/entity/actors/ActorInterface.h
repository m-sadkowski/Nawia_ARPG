#pragma once

#include <EntityBuilder.h>

#include <memory>

namespace Nawia::Core {
	class Map;
}

namespace Nawia::Entity {

	/**
	 * @class ActorInterface
	 * @brief Wspólna baza dla jednostek bojowych korzystających z mapy i celu.
	 *
	 * Klasa zbiera zachowanie współdzielone przez wrogów i sojuszników, dzięki
	 * czemu klasy `EnemyInterface` oraz `AllyInterface` opisują już tylko własną
	 * specjalizację frakcji lub logiki.
	 */
	class ActorInterface : public Entity {
	public:
		/**
		 * @brief Ustawia aktualny cel aktora.
		 * @param target Encja, którą aktor ma śledzić lub atakować.
		 */
		void setTarget(const std::shared_ptr<Entity>& target) override { Entity::setTarget(target); }

		/**
	 * @brief Podpina mapę używaną przez ruch, wyznaczanie ścieżki i walidację pozycji.
		 * @param map Mapa aktywnego poziomu; może być pusta dla prostych aktorów.
		 */
		void setMap(Core::Map* map) { _map = map; }

		/**
		 * @brief Zwraca mapę przypisaną do aktora.
		 */
		[[nodiscard]] Core::Map* getMap() const { return _map; }

	protected:
		template <typename T> friend class ActorBuilder;

		/** @brief Konstruktor dla builderów ustawiających stan krok po kroku. */
		ActorInterface() = default;

		/**
		 * @brief Tworzy aktora z podstawowymi danymi encji i mapą poziomu.
		 */
		ActorInterface(
			const std::string& name,
			float x,
			float y,
			const std::shared_ptr<Texture2D>& texture,
			int max_hp,
			Core::Map* map)
			: Entity(name, x, y, texture, max_hp), _map(map)
		{
		}

		Core::Map* _map = nullptr;
	};

	/**
	 * @class ActorBuilder
	 * @brief Wspólne kroki budowania aktorów: mapa i target.
	 */
	template <typename Derived>
	class ActorBuilder : public EntityBuilder<Derived> {
	public:
		ActorBuilder() = default;

		/**
		 * @brief Ustawia mapę aktora podczas składania obiektu.
		 */
		Derived& setMap(Core::Map* map) {
			auto actor_ptr = static_cast<ActorInterface*>(this->_entity);
			actor_ptr->_map = map;
			return this->self();
		}

		/**
		 * @brief Ustawia początkowy cel aktora podczas składania obiektu.
		 */
		Derived& setTarget(const std::shared_ptr<Entity>& target) {
			auto actor_ptr = static_cast<ActorInterface*>(this->_entity);
			actor_ptr->setTarget(target);
			return this->self();
		}
	};

} // namespace Nawia::Entity
