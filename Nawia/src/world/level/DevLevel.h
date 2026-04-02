#pragma once

#include "Level.h"

#include <string>
#include <memory>
#include <raylib.h>

namespace Nawia::World {

	class DevLevel : public Level {
	public:
		DevLevel();
		~DevLevel() override;

		void onEnter(Core::Engine* engine) override;
		void onExit(Core::Engine* engine) override;
		
		void handleInput(Core::Engine* engine) override;
		void update(Core::Engine* engine, float dt) override;
		void renderUI(Core::Engine* engine) override;

		[[nodiscard]] Core::Map* getMap() const override;
		[[nodiscard]] std::string getName() const override { return "DevLevel"; }
		
		[[nodiscard]] bool isTyping() const { return _is_typing; }

	private:
		std::unique_ptr<Core::Map> _map;
		
		bool _is_typing;
		std::string _input_text;
		Vector2 _saved_iso_pos;
		
		void saveObjectToJson(const std::string& name, float x, float y);
	};

} // namespace Nawia::World
