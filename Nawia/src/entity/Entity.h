#pragma once
#include "AbilityStats.h"

#include <MathUtils.h>

#include <raylib.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace Nawia::Entity {
	class Ability;
	class Collider;

	class Entity {
	public:
		Entity(const std::string& name, float start_x, float start_y, const std::shared_ptr<Texture2D>& texture, int max_hp);
		virtual ~Entity();

		virtual void update(float delta_time);
		virtual void render(float offset_x, float offset_y);


		[[nodiscard]] float getX() const { return _pos.x; }
		[[nodiscard]] float getY() const { return _pos.y; }
		void setX(float x) { _pos.x = x; }
		void setY(float y) { _pos.y = y; }
		[[nodiscard]] Vector2 getCenter() const;

		[[nodiscard]] Vector2 getScreenPos(float mouse_x, float mouse_y, float cam_x, float cam_y) const;
		[[nodiscard]] Vector2 getIsoPos(float world_x, float world_y, float cam_x, float cam_y) const;
		[[nodiscard]] virtual bool isMouseOver(float mouse_x, float mouse_y, float cam_x, float cam_y) const;

		// Transform & Physics
		void setVelocity(const float x, const float y) { _velocity.x = x; _velocity.y = y; }
		[[nodiscard]] Vector2 getVelocity() const { return _velocity; }
		void setScale(const float scale) { _scale = scale; }
		[[nodiscard]] float getScale() const { return _scale; }

		// Damage and HP
		virtual void takeDamage(int dmg);
		void die();
		[[nodiscard]] bool isDead() const { return _hp <= 0; }
		[[nodiscard]] int getHP() const { return _hp; }
		[[nodiscard]] int getMaxHP() const { return _max_hp; }
		[[nodiscard]] std::string getName() const { return _name; }
		void setName(const std::string& name) { _name = name; }

		// Animation & 3D Model Support
		void loadModel(const std::string& path, bool rotate_model = false);
		void addAnimation(const std::string& name, const std::string& path);
		void playAnimation(const std::string& name, bool loop = true, bool lock_movement = false);
		[[nodiscard]] int getAnimationFrameCount(const std::string& name) const;
		[[nodiscard]] bool isAnimationLocked() const { return _anim_locked; }

		void setRotation(const float angle) { _rotation = angle; }
		[[nodiscard]] float getRotation() const { return _rotation; }
		void rotateTowards(float world_x, float world_y);

		// Collider System
		void setCollider(std::unique_ptr<Collider> collider);
		[[nodiscard]] Collider* getCollider() const { return _collider.get(); }
		static bool DebugColliders; // Static flag for debug drawing

		// Ability System
		static AbilityStats getAbilityStatsFromJson(const std::string& name);
		void addAbility(const std::shared_ptr<Ability>& ability);
		[[nodiscard]] std::shared_ptr<Ability> getAbility(int index);
		void updateAbilities(float dt) const;


		// Spawning Support
		// mechanisms for entities to spawn other entities (e.g. projectiles) safely during the update loop
		void addPendingSpawn(const std::shared_ptr<Entity>& entity) { _pending_spawns.push_back(entity); }
		[[nodiscard]] std::vector<std::shared_ptr<Entity>> getPendingSpawns() { return _pending_spawns; }
		void clearPendingSpawns() { _pending_spawns.clear(); }

		// Faction System
		enum class Faction {
			Player,
			Enemy,
			Neutral,
			Ally,
			None
		};

		[[nodiscard]] Faction getFaction() const { return _faction; }
		void setFaction(Faction faction) { _faction = faction; }

	protected:
		Vector2 _pos;
		Vector2 _velocity;
		float _scale;
		std::shared_ptr<Texture2D> _texture;
		
		std::unique_ptr<Collider> _collider;
		
		std::vector<std::shared_ptr<Entity>> _pending_spawns;

		int _hp;
		int _max_hp;

		// 3D Model & Animation Data
		Model _model;
		std::vector<ModelAnimation> _animations;
		std::map<std::string, int> _animation_map;
		
		int _current_anim_index;
		int _anim_frame_counter;
		float _rotation;
		bool _model_loaded;
		bool _anim_looping;
		bool _anim_locked;

		Faction _faction;

		std::string _name;

		// 3D Rendering Support
		RenderTexture2D _target;
		Camera3D _camera;
		bool _use_3d_rendering;

		void updateAnimation(float dt);
		
		std::vector<std::shared_ptr<Ability>> _abilities;
	};

} // namespace Nawia::Entity