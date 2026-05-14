#pragma once

#include <Backpack.h>
#include <Entity.h>
#include <Equipment.h>
#include <Stats.h>

#include <memory>
#include <string>
#include <vector>

namespace Nawia::Core {
	class Engine;
}

namespace Nawia::Entity {

	/**
	 * @class Player
	 * @brief Grywalna encja przechowująca ekwipunek, statystyki i obsługę ruchu.
	 */
	class Player : public Entity {
	public:
		/** @brief Zwraca silnik, do którego należy gracz. */
		Core::Engine* getEngine() const { return _engine; }

		/** @brief Aktualizuje ruch, animacje, ekwipunek i stan powalenia gracza. */
		void update(float delta_time) override;

		/** @brief Zadaje obrażenia z uwzględnieniem statystyk gracza. */
		void takeDamage(int dmg) override;

		/** @brief Zwraca, czy gracz jest w sekwencji powalenia. */
		[[nodiscard]] bool isKnockedDown() const { return _is_knocked_down; }

		/** @brief Ustawia cel ruchu gracza. */
		void moveTo(float x, float y) override;

		/** @brief Natychmiast zatrzymuje ruch gracza. */
		void stop();

		/** @brief Przesuwa gracza po ścieżce wyznaczonej przez mapę. */
		void updateMovement(float delta_time) override;

		/** @brief Ustawia punkt odrodzenia gracza. */
		void setRespawnPoint(const Vector2& point) { _respawn_point = point; }

		/** @brief Zwraca aktualny punkt odrodzenia gracza. */
		[[nodiscard]] Vector2 getRespawnPoint() const { return _respawn_point; }

		/** @brief Przywraca gracza do życia w punkcie odrodzenia. */
		void respawn();

		/**
		 * @brief Powala gracza, np. po trafieniu doskokiem Devila.
		 *
		 * Odtwarza animację powalenia, potem animację wstawania, a ruch blokuje
		 * przez całą sekwencję.
		 * @param damage Obrażenia zadawane przy powaleniu.
		 */
		void knockDown(int damage);

		/** @brief Zakłada przedmiot z plecaka, jeśli pasuje do slotu ekwipunku. */
		void equipItemFromBackpack(int backpack_index);

		/** @brief Zaklada podany przedmiot bezposrednio do ekwipunku. */
		bool equipItem(const std::shared_ptr<Item::Item>& item);

		/** @brief Zdejmuje przedmiot z wybranego slotu ekwipunku. */
		void unequipItem(Item::EquipmentSlot slot);

		/** @brief Zwraca plecak gracza tylko do odczytu. */
		[[nodiscard]] const Item::Backpack& getBackpack() const { return *_backpack; }

		/** @brief Zwraca modyfikowalny plecak gracza. */
		Item::Backpack& getBackpack() { return *_backpack; }

		/** @brief Zwraca aktualne wyposażenie gracza. */
		[[nodiscard]] const Item::Equipment& getEquipment() const { return *_equipment; }

		/** @brief Zwraca ilość złota gracza. */
		[[nodiscard]] int getGold() const { return _gold; }

		/** @brief Dodaje złoto do portfela gracza. */
		void addGold(int amount) { _gold += amount; }

		/** @brief Próbuje wydać złoto i zwraca, czy operacja się udała. */
		bool spendGold(int amount) {
			if (_gold >= amount) {
				_gold -= amount;
				return true;
			}

			return false;
		}

		/** @brief Przelicza statystyki z bazowych wartości i ekwipunku. */
		void recalculateStats();

		/** @brief Zwraca aktualne statystyki po przeliczeniu bonusów. */
		[[nodiscard]] const Stats& getStats() const { return _current_stats; }

		/** @brief Zwraca aktualny poziom gracza. */
		[[nodiscard]] int getLevel() const { return _level; }

		/** @brief Ustawia poziom gracza. */
		void setLevel(int level) { _level = level; }

		/** @brief Zwraca bieżącą liczbę punktów doświadczenia. */
		[[nodiscard]] int getExp() const { return _exp; }

		/** @brief Dodaje punkty doświadczenia. */
		void addExp(int amount) { _exp += amount; }

		/** @brief Zwraca próg doświadczenia wymagany do kolejnego poziomu. */
		[[nodiscard]] int getExpToNextLvl() const { return _exp_to_next_lvl; }

		/** @brief Podnosi poziom gracza i zwiększa próg doświadczenia. */
		void levelUp();

		/** @brief Sprawdza, czy gracz zebrał dość doświadczenia na poziom. */
		void isLevelUp();

		// Stałe prędkości animacji.
		static constexpr float DEFAULT_ANIMATION_SPEED = 1.0f;
		static constexpr float WALK_ANIM_BASE_SPEED = 1.25f;
		static constexpr float ATTACK_ANIM_BASE_SPEED = 1.5f;

	private:
		friend class PlayerBuilder;
		Player();
		void onDeathStarted() override;
		void attachEngine(Core::Engine* engine);
		void updateWeaponVisualModel();

		Core::Engine* _engine = nullptr;

		static constexpr int INIT_BACKPACK_SIZE = 20;
		bool _is_knocked_down = false;

		enum class KnockdownPhase {
			None,
			Knocked,
			StandingUp
		};

		KnockdownPhase _knockdown_phase = KnockdownPhase::None;
		std::vector<Vector2> _path;

		Vector2 _respawn_point = {0.0f, 0.0f};

		std::unique_ptr<Item::Backpack> _backpack;

		Stats _base_stats;
		Stats _current_stats;

		int _gold = 0;
		int _level = 1;
		int _exp = 0;
		int _exp_to_next_lvl = 100;
		std::string _active_visual_model_path;
	};

	/**
	 * @class PlayerBuilder
	 * @brief Builder składający instancję gracza i podpinający silnik.
	 */
	class PlayerBuilder : public EntityBuilder<PlayerBuilder> {
	public:
		/** @brief Tworzy roboczego gracza dla podanego silnika. */
		explicit PlayerBuilder(Core::Engine* engine) {
			_player_ptr = std::unique_ptr<Player>(new Player());
			_player_ptr->attachEngine(engine);

			this->_entity = _player_ptr.get();
		}

		/** @brief Ustawia pozycję oraz początkowy cel ruchu i punkt odrodzenia. */
		PlayerBuilder& setPosition(Vector2 pos) {
			EntityBuilder<PlayerBuilder>::setPosition(pos);
			_player_ptr->_target_x = pos.x;
			_player_ptr->_target_y = pos.y;
			_player_ptr->_respawn_point = pos;
			return *this;
		}

		/** @brief Ustawia współrzędną X oraz cel ruchu na osi X. */
		PlayerBuilder& setX(float x) {
			EntityBuilder<PlayerBuilder>::setX(x);
			_player_ptr->_target_x = x;
			return *this;
		}

		/** @brief Ustawia współrzędną Y oraz cel ruchu na osi Y. */
		PlayerBuilder& setY(float y) {
			EntityBuilder<PlayerBuilder>::setY(y);
			_player_ptr->_target_y = y;
			return *this;
		}

		/** @brief Oddaje gotową instancję gracza. */
		std::unique_ptr<Player> build() {
			return std::move(_player_ptr);
		}

	private:
		std::unique_ptr<Player> _player_ptr;
	};

} // namespace Nawia::Entity
