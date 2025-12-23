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

	class Entity {
	public:
		Entity(float start_x, float start_y, const std::shared_ptr<Texture2D>& texture, int max_hp);
		virtual ~Entity();

		virtual void update(float delta_time);
		virtual void render(float offset_x, float offset_y);


		[[nodiscard]] float getX() const { return _pos->getX(); }
		[[nodiscard]] float getY() const { return _pos->getY(); }
		void setX(float x) { _pos->setX(x); }
		void setY(float y) { _pos->setY(y); }

		[[nodiscard]] Core::Point2D getScreenPos(float mouse_x, float mouse_y, float cam_x, float cam_y) const;
		[[nodiscard]] virtual bool isMouseOver(float mouse_x, float mouse_y, float cam_x, float cam_y) const;

		// Transform & Physics
		void setVelocity(float x, float y) { _velocity.setX(x); _velocity.setY(y); }
		[[nodiscard]] Core::Point2D getVelocity() const { return _velocity; }
		void setScale(float scale) { _scale = scale; }
		[[nodiscard]] float getScale() const { return _scale; }

		virtual void takeDamage(int dmg);
		void die();
		[[nodiscard]] bool isDead() const { return _hp <= 0; }
		[[nodiscard]] int getHP() const { return _hp; }
		[[nodiscard]] int getMaxHP() const { return _max_hp; }
		[[nodiscard]] std::string getName() { return ""; };
		
		static AbilityStats getAbilityStatsFromJson(const std::string& name);

		// Animation & 3D Model Support
		void loadModel(const std::string& path, bool rotate_model = false);
		void addAnimation(const std::string& name, const std::string& path);
		void playAnimation(const std::string& name);
		void setRotation(float angle) { _rotation = angle; }

	protected:
		std::unique_ptr<Core::Point2D> _pos;
		Core::Point2D _velocity;
		float _scale;
		std::shared_ptr<Texture2D> _texture;

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

		// 3D Rendering Support
		RenderTexture2D _target;
		Camera3D _camera;
		bool _use_3d_rendering;

		void updateAnimation(float dt);
	};

} // namespace Nawia::Entity