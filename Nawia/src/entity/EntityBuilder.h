#pragma once

#include <Entity.h>

namespace Nawia::Entity {

	template <typename Derived>
	class EntityBuilder {
	public:
		EntityBuilder() = default;

		Derived& setName(const std::string& name) {
			_entity->_name = name;
			return self();
		}

		Derived& setTexture(const std::shared_ptr<Texture>& texture) {
			_entity->_texture = texture;
			return self();
		}

		Derived& setMovementSpeed(const float speed) {
			_entity->setMovementSpeed(speed);
			return self();
		}

		Derived& setRotation(const float rotation) {
			_entity->setRotation(rotation);
			return self();
		}

		Derived& setAudioManager(Audio::AudioManager* audio_manager) {
			_entity->_audio_manager = audio_manager;
			return self();
		}

		Derived& setPosition(const Vector2 pos) {
			_entity->setPosition(pos);
			return self();
		}

		Derived& setX(const float x) {
			_entity->setX(x);
			return self();
		}

		Derived& setY(const float y) {
			_entity->setY(y);
			return self();
		}

		Derived& setMaxHp(const int max_hp) {
			_entity->_max_hp = max_hp;
			_entity->_hp = max_hp;
			return self();
		}

	protected:
		Entity* _entity = nullptr;

		Derived& self() {
			return static_cast<Derived&>(*this);
		}
	};

} // namespace Nawia::Entity
