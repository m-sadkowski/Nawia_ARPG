#pragma once

#include <raylib.h>

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace Nawia::Entity { class Entity; }

namespace Nawia::Core {

	class Engine;

	/**
	 * @class EntityManager
	 * @brief Przechowuje aktywne encje i koordynuje ich update, render oraz kolizje.
	 *
	 * Manager posiada encje przez `shared_ptr`, bo te same obiekty sa widziane
	 * przez kilka systemow gameplayowych. `_engine` jest nieposiadajacym
	 * wskaznikiem do wlasciciela managera.
	 */
	class EntityManager {
	public:
		explicit EntityManager(Engine* engine) : _engine(engine) {}
		~EntityManager() = default;

		/**
		 * @brief Zwraca aktywne encje do renderowania i UI.
		 */
		[[nodiscard]] const std::vector<std::shared_ptr<Entity::Entity>>& getEntities() const { return _active_entities; }

		/**
		 * @brief Dodaje encje do aktywnej listy.
		 */
		void addEntity(std::shared_ptr<Entity::Entity> new_entity);

		/**
		 * @brief Usuwa konkretna encje z aktywnej listy.
		 */
		void removeEntity(const std::shared_ptr<Entity::Entity>& entity);

		/**
		 * @brief Ustawia gracza przechowywanego przez manager.
		 */
		void setPlayer(std::shared_ptr<Entity::Entity> player);

		/**
		 * @brief Usuwa wszystkie encje poza graczem.
		 */
		void clearNonPlayerEntities();

	private:
		void assignEntityIdIfMissing(const std::shared_ptr<Entity::Entity>& entity);
		void updateEntities(float delta_time);
		void renderEntities(const Camera3D& camera) const;
		void handleEntitiesCollisions() const;
		void refreshCombatTargets();

		[[nodiscard]] std::shared_ptr<Entity::Entity> getEntityAt(float screen_x, float screen_y, const Camera3D& camera) const;
		[[nodiscard]] std::shared_ptr<Entity::Entity> getHoveredEntity() const { return _hovered_entity.lock(); }
		void updateHoverState(float screen_x, float screen_y, const Camera3D& camera);

		void processAbilityCollisions() const;
		void processTriggerCollisions() const;
		void processPhysicalCollisions() const;

		[[nodiscard]] bool isCollidablePhysicalEntity(const std::shared_ptr<Entity::Entity>& entity) const;
		void resolveOverlap(const std::shared_ptr<Entity::Entity>& first_entity, const std::shared_ptr<Entity::Entity>& second_entity) const;
		[[nodiscard]] std::shared_ptr<Entity::Entity> findClosestCombatTarget(const std::shared_ptr<Entity::Entity>& seeker) const;

		Engine* _engine = nullptr;
		std::uint64_t _next_entity_id = 1;
		std::vector<std::shared_ptr<Entity::Entity>> _active_entities;
		std::shared_ptr<Entity::Entity> _player;
		std::weak_ptr<Entity::Entity> _hovered_entity;
		float _combat_target_refresh_timer = 0.0f;
		float _altitude_snap_timer = 0.0f;

		friend class Engine;
	};

} // namespace Nawia::Core
