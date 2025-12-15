#pragma once
#include "Entity.h"
#include "Spell.h"

namespace Nawia::Entity {

	class Player : public Entity {
	public:
		Player(float x, float y, const std::shared_ptr<SDL_Texture>& texture);

		void update(float delta_time) override;

		void moveTo(float x, float y);

		void addSpell(const std::shared_ptr<Core::Spell>& spell);
		std::shared_ptr<Core::Spell> getSpell(int index);
		void updateSpells(float dt) const;

	private:
		float _target_x, _target_y;
		float _speed;
		bool _is_moving;
		std::vector<std::shared_ptr<Core::Spell>> _spellbook;
	};

} // namespace Nawia::Entity