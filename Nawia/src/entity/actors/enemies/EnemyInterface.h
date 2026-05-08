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
	public:
		void setSpeedMultiplier(float m) { _speed_multiplier = m; }
		[[nodiscard]] float getSpeedMultiplier() const { return _speed_multiplier; }
		void setDamageMultiplier(float m) { _damage_multiplier = m; }
		[[nodiscard]] float getDamageMultiplier() const { return _damage_multiplier; }

	protected:
		template <typename T> friend class EnemyBuilder;
		EnemyInterface() {
			_type = EntityType::Enemy;
		}

		EnemyInterface(const std::string& name, float x, float y, const std::shared_ptr<Texture2D>& texture, int max_hp, Core::Map* map)
			: ActorInterface(name, x, y, texture, max_hp, map) {
			_type = EntityType::Enemy;
		}

		float _speed_multiplier = 1.0f;
		float _damage_multiplier = 1.0f;
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
