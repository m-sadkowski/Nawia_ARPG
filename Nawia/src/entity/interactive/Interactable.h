#pragma once

namespace Nawia::Entity {

	class Entity;

	/**
	 * @interface Interactable
	 * @brief Interfejs obiektów, z którymi gracz może wejść w interakcję.
	 *
	 * Implementuj go dla encji reagujących na kliknięcie albo wejście w obszar
	 * aktywujący. Obiekty klikalne zwykle dziedziczą po `InteractiveClickable`,
	 * a strefy aktywowane kolizją implementują `onTriggerEnter()`.
	 *
	 * @note Interakcje wymagają kolidera do wykrywania zasięgu lub obszaru aktywującego.
	 */
	class Interactable {
	public:
		virtual ~Interactable() = default;

		/**
		 * @brief Wywoływane, gdy encja wchodzi w interakcję z obiektem.
		 * @param instigator Encja inicjująca interakcję.
		 */
		virtual void onInteract(Entity& instigator) = 0;

		/**
		 * @brief Wywoływane, gdy encja wchodzi w obszar aktywujący obiektu.
		 * @param other Encja, która weszła w obszar aktywujący.
		 */
		virtual void onTriggerEnter(Entity& other) = 0;

		/**
		 * @brief Sprawdza, czy interakcja jest obecnie możliwa.
		 * @return `true`, jeśli można wejść w interakcję.
		 */
		virtual bool canInteract() const { return true; }

		/**
		 * @brief Zwraca maksymalny zasięg interakcji.
		 * @return Dystans w jednostkach świata.
		 */
		virtual float getInteractionRange() = 0;
	};

} // namespace Nawia::Entity
