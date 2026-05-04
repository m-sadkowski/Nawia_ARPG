#pragma once

namespace Nawia::Entity {

	/**
	 * @struct Stats
	 * @brief Statystyki bojowe i ruchu używane przez gracza oraz wyposażenie.
	 */
	struct Stats {
		int max_hp = 0;              ///< Maksymalna liczba punktów życia.
		int damage = 0;              ///< Bazowe obrażenia fizyczne.
		int power = 0;               ///< Siła umiejętności.
		float attack_speed = 0.0f;   ///< Mnożnik prędkości ataku.
		float movement_speed = 0.0f; ///< Bonus do prędkości ruchu.
		int tenacity = 0;            ///< Redukcja obrażeń lub odporność na efekty.
		int armor = 0;               ///< Pancerz zmniejszający otrzymywane obrażenia.

		/**
		 * @brief Zwraca sumę dwóch zestawów statystyk.
		 */
		Stats operator+(const Stats& other) const {
			Stats result;
			result.max_hp = max_hp + other.max_hp;
			result.damage = damage + other.damage;
			result.attack_speed = attack_speed + other.attack_speed;
			result.movement_speed = movement_speed + other.movement_speed;
			result.power = power + other.power;
			result.tenacity = tenacity + other.tenacity;
			result.armor = armor + other.armor;
			return result;
		}

		/**
		 * @brief Dodaje drugi zestaw statystyk do bieżącego obiektu.
		 */
		Stats& operator+=(const Stats& other) {
			max_hp += other.max_hp;
			damage += other.damage;
			attack_speed += other.attack_speed;
			movement_speed += other.movement_speed;
			power += other.power;
			tenacity += other.tenacity;
			armor += other.armor;
			return *this;
		}
	};

} // namespace Nawia::Entity
