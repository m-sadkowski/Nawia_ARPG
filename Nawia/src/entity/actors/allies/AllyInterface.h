#pragma once

#include <ActorInterface.h>

#include <memory>

namespace Nawia::Entity {

	class AllyBrain;

	/**
	 * @class AllyInterface
	 * @brief Baza dla wszystkich sojuszników.
	 *
	 * Klasa dodaje obsługę opcjonalnego obiektu decyzyjnego. Mapę i wybieranie celu bierze ze
	 * wspólnego `ActorInterface`, tak samo jak klasy wrogów.
	 */
	class AllyInterface : public ActorInterface {
	public:
		/**
		 * @brief Ustawia obiekt decyzyjny sterujący zachowaniem sojusznika.
		 */
		void setBrain(const std::shared_ptr<AllyBrain>& brain) { _brain = brain; }

		/**
		 * @brief Zwraca brain, jeśli sojusznik korzysta z delegowanej logiki AI.
		 */
		[[nodiscard]] std::shared_ptr<AllyBrain> getBrain() const { return _brain; }

	protected:
		template <typename T> friend class AllyBuilder;
		AllyInterface() {
			_type = EntityType::Ally;
		}

		AllyInterface(const std::string& name, float x, float y, const std::shared_ptr<Texture2D>& texture, int max_hp, Core::Map* map)
			: ActorInterface(name, x, y, texture, max_hp, map) {
			_type = EntityType::Ally;
		}

		std::shared_ptr<AllyBrain> _brain = nullptr;
	};

	/**
	 * @class AllyBuilder
	 * @brief Bazowy builder dla sojuszników, rozszerzony o konfigurację braina.
	 */
	template <typename Derived>
	class AllyBuilder : public ActorBuilder<Derived> {
	public:
		AllyBuilder() = default;

		/**
		 * @brief Podpina brain sterujący logiką sojusznika.
		 */
		Derived& setBrain(const std::shared_ptr<AllyBrain>& brain) {
			auto ally_ptr = static_cast<AllyInterface*>(this->_entity);
			ally_ptr->_brain = brain;
			return this->self();
		}
	};

} // namespace Nawia::Entity
