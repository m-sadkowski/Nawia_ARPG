#pragma once

namespace Nawia::Entity {

	class AllyInterface;

	/**
	 * @class AllyBrain
	 * @brief Interfejs strategii sterującej zachowaniem sojusznika.
	 */
	class AllyBrain {
	public:
		virtual ~AllyBrain() = default;

		/**
		 * @brief Aktualizuje decyzje sojusznika w danej klatce.
		 * @param ally Sojusznik, którym steruje brain.
		 * @param dt Czas od poprzedniej klatki w sekundach.
		 */
		virtual void update(AllyInterface& ally, float dt);
	};

} // namespace Nawia::Entity
