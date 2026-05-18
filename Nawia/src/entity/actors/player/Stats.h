#pragma once

namespace Nawia::Entity {

	/**
	 * @struct Stats
	 * @brief Statystyki bojowe i ruchu uzywane przez gracza oraz wyposazenie.
	 */
	struct Stats {
		int max_hp = 0;              ///< Maksymalna liczba punktow zycia.
		int damage = 0;              ///< Bazowe obrazenia fizyczne.
		int power = 0;               ///< Sila umiejetnosci.
		float attack_speed = 0.0f;   ///< Mnoznik predkosci ataku.
		float movement_speed = 0.0f; ///< Bonus do predkosci ruchu.
		int defense = 0;             ///< Obrona zmniejszajaca otrzymywane obrazenia.

		/**
		 * @brief Zwraca sume dwoch zestawow statystyk.
		 */
		Stats operator+(const Stats& other) const {
			Stats result;
			result.max_hp = max_hp + other.max_hp;
			result.damage = damage + other.damage;
			result.attack_speed = attack_speed + other.attack_speed;
			result.movement_speed = movement_speed + other.movement_speed;
			result.power = power + other.power;
			result.defense = defense + other.defense;
			return result;
		}

		/**
		 * @brief Dodaje drugi zestaw statystyk do biezacego obiektu.
		 */
		Stats& operator+=(const Stats& other) {
			max_hp += other.max_hp;
			damage += other.damage;
			attack_speed += other.attack_speed;
			movement_speed += other.movement_speed;
			power += other.power;
			defense += other.defense;
			return *this;
		}
	};

} // namespace Nawia::Entity
