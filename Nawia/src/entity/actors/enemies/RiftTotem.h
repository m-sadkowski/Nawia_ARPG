#pragma once

#include <EnemyInterface.h>

#include <memory>
#include <string>

namespace Nawia::Entity {

	/**
	 * @brief Destructible stage totem used by the Siewca Chaosu boss fight.
	 *
	 * Totems are regular Enemy entities so player attacks and future agent
	 * commands can target them without a separate interaction path. Each totem
	 * can summon one helper while it protects the boss.
	 */
	class RiftTotem : public EnemyInterface {
	public:
		RiftTotem(
			float x,
			float y,
			Core::Map* map,
			std::weak_ptr<Entity> owner,
			std::shared_ptr<Entity> target,
			int stage_index,
			std::string helper_model_path,
			float helper_model_scale);

		void update(float dt) override;
		void render(const Camera3D& camera) override;
		void takeDamage(int dmg) override;
		[[nodiscard]] bool shouldWakeOnLocationChange() const override { return false; }

	private:
		void spawnHelper();
		void renderTether() const;
		[[nodiscard]] Vector2 findHelperSpawnPosition() const;

		std::weak_ptr<Entity> _owner;
		std::string _helper_model_path;
		float _helper_model_scale = 1.5f;
		int _stage_index = 0;
		float _helper_spawn_timer = 1.0f;
		bool _helper_spawned = false;

		static constexpr int BASE_HP = 65;
		static constexpr int HP_PER_STAGE = 18;
		static constexpr int HELPER_BASE_HP = 50;
		static constexpr int HELPER_HP_PER_STAGE = 10;
		static constexpr float HELPER_SPAWN_RADIUS = 2.15f;
	};

} // namespace Nawia::Entity
