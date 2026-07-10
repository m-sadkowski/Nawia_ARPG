#pragma once

#include <AllyInterface.h>

#include <memory>
#include <vector>

namespace Nawia::Entity {

	/**
	 * @class Friend
	 * @brief Prosty sojusznik walczący obok gracza.
	 */
	class Friend : public AllyInterface {
	public:
		/**
		 * @brief Aktualizuje AI, ruch, animacje i umiejętności sojusznika.
		 */
		void update(float dt) override;

	private:
		Friend();
		friend class FriendBuilder;

		static constexpr float VISION_RANGE = 16.0f;
		static constexpr float ATTACK_RANGE_MULTIPLIER = 0.5f;
		static constexpr float SPEED = 3.5f;
		static constexpr float PATH_RECALC_INTERVAL = 0.35f;

		/** @brief Obsługuje tymczasową, zaszytą w klasie logikę walki. */
		void updateHardcodedBehavior(float dt);
		void stopPathMovement();
		void rebuildPathToTarget(const Entity& target);
		void updatePathMovement(float dt);
		void onDeathStarted() override;

		std::vector<Vector2> _current_path;
	};

	/**
	 * @class FriendBuilder
	 * @brief Buduje instancję klasy `Friend` zgodnie z builderami aktorów.
	 */
	class FriendBuilder : public AllyBuilder<FriendBuilder> {
	public:
		/** @brief Tworzy roboczą instancję sojusznika. */
		FriendBuilder() {
			_friend_ptr = std::unique_ptr<Friend>(new Friend());
			this->_entity = _friend_ptr.get();
		}

		/** @brief Oddaje gotową instancję sojusznika. */
		std::unique_ptr<Friend> build() {
			return std::move(_friend_ptr);
		}

	private:
		std::unique_ptr<Friend> _friend_ptr;
	};

} // namespace Nawia::Entity
