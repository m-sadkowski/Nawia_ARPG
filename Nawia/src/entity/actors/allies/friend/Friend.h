#pragma once

#include "AllyInterface.h"

namespace Nawia::Entity {

	class Friend : public AllyInterface {
	public:
		void update(float dt) override;

	private:
		Friend();
		friend class FriendBuilder;

		static constexpr float VISION_RANGE = 16.0f;
		static constexpr float ATTACK_RANGE_MULTIPLIER = 0.5f;
		static constexpr float SPEED = 3.5f;

		void updateHardcodedBehavior(float dt);
	};

	class FriendBuilder : public AllyBuilder<FriendBuilder> {
	public:
		FriendBuilder() {
			_friend_ptr = std::unique_ptr<Friend>(new Friend());
			this->_entity = _friend_ptr.get();
		}

		std::unique_ptr<Friend> build() {
			return std::move(_friend_ptr);
		}

	private:
		std::unique_ptr<Friend> _friend_ptr;
	};

} // namespace Nawia::Entity
