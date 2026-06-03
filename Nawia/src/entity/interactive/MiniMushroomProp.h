#pragma once

#include <Entity.h>

namespace Nawia::Entity {

	/**
	 * @brief Wizualny mini-grzyb bez logiki walki.
	 *
	 * Uzywany po oczyszczeniu/uratowaniu, zeby w swiecie zostal zywy obiekt
	 * bez AI przeciwnika. Aktualizacja steruje tylko mala animacja skoku w idle.
	 */
	class MiniMushroomProp : public Entity {
	public:
		MiniMushroomProp();

		void update(float delta_time) override;

	private:
		bool _jumping = false;
		float _jump_timer = 2.0f; ///< Odliczanie miedzy dekoracyjnymi podskokami.
	};

} // namespace Nawia::Entity
