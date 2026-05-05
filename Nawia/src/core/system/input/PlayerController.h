#pragma once

#include <Player.h>
#include <Interactable.h>

#include <raylib.h>
#include <memory>

namespace Nawia::Core {

	class Engine;

	class PlayerController {
	public:
		PlayerController(Engine *engine, std::shared_ptr<Entity::Player> player);

		void handleInput(Vector3 mouse_world_pos, float screen_x, float screen_y);
		void update(float dt);
		
	private:
		Engine* _engine;
		std::shared_ptr<Entity::Player> _player;
		std::shared_ptr<Entity::Entity> _target_enemy;
		std::shared_ptr<Entity::Interactable> _target_interactable;
		void useAbility(int index, float target_x, float target_y) const;

		void handleMouseInput(Vector3 mouse_world_pos, float screen_x, float screen_y);
		void handleKeyboardInput(Vector3 mouse_world_pos, float screen_x, float screen_y);
		void processPendingAction();
		void processAutoAttack();
		bool processInteraction();
		void updateRotation() const;

		bool trySelectEnemy(float screen_x, float screen_y);
		void handleGroundClick(Vector3 pos);
		void queueAbility(int index, float x, float y, float screen_x, float screen_y);
		void castAbility(int index, float x, float y, float screen_x, float screen_y);
		void processPendingMove();
		void processPendingAbility() const;
		void updateCombatMovement(float dist_sq, float attack_range) const;
		void updatePathMovement();
		bool moveTowardInteractable(const std::shared_ptr<Entity::Entity>& target, float interaction_range_sq);
		bool buildPathToWorldPosition(Vector3 desired_world_position);
		void trimCurrentPathStart();
		void moveAlongCurrentPath();

		std::vector<Vector2> _current_path;

		struct PendingAction 
		{
			enum class Type { None, Move, Ability, Interact } type = Type::None;
			float x = 0.0f;
			float y = 0.0f;
			float world_height = 0.0f;
			int ability_index = -1;
			std::weak_ptr<Entity::Entity> target;
		};

		PendingAction _pending_action;
		float _last_mouse_x = 0.0f;
		float _last_mouse_y = 0.0f;
	};

} // namespace Nawia::Core
