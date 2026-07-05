#pragma once

#include <ActorInterface.h>

namespace Nawia::Entity {

	/**
	 * @class EnemyInterface
	 * @brief Baza dla wszystkich wrogów.
	 *
	 * Klasa ustawia typ encji na `Enemy`; mapę i wybieranie celu dziedziczy po
	 * `ActorInterface`, żeby nie duplikować tego samego kodu w ally i enemy.
	 */
	class EnemyInterface : public ActorInterface {
	protected:
		template <typename T> friend class EnemyBuilder;
		EnemyInterface() {
			setType(EntityType::Enemy);
		}

		EnemyInterface(const std::string& name, float x, float y, const std::shared_ptr<Texture2D>& texture, int max_hp, Core::Map* map)
			: ActorInterface(name, x, y, texture, max_hp, map) {
			setType(EntityType::Enemy);
		}
	};

	/**
	 * @class EnemyBuilder
	 * @brief Bazowy builder dla klas wrogów.
	 */
	template <typename Derived>
	class EnemyBuilder : public ActorBuilder<Derived> {
	public:
		EnemyBuilder() = default;
	};

} // namespace Nawia::Entity
