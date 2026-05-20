#pragma once

#include <InteractiveTrigger.h>

#include <string>

namespace Nawia::Entity {

	/**
	 * @class Checkpoint
	 * @brief Trigger zapisujący postęp po wejściu gracza w jego obszar.
	 */
	class Checkpoint : public InteractiveTrigger {
	public:
		/** @brief Tworzy checkpoint w podanym punkcie mapy. */
		Checkpoint(const std::string& name, float x, float y);

		/** @brief Aktywuje checkpoint, gdy wejdzie w niego gracz. */
		void onTriggerEnter(Entity& other) override;

		/** @brief Aktualizuje bazowy stan encji. */
		void update(float delta_time) override;

		/** @brief Renderuje checkpoint w trybie diagnostycznym. */
		void render(const Camera3D& camera) override;

		/** @brief Zwraca, czy checkpoint został już aktywowany. */
		[[nodiscard]] bool isActivated() const { return _activated; }
		void setActivated(bool activated) { _activated = activated; }

		/** @brief Zwraca zasięg interakcji triggera. */
		float getInteractionRange() override;

	private:
		bool _activated = false;
	};

} // namespace Nawia::Entity
