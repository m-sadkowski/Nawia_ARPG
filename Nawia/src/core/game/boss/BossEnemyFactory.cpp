#include "BossEnemyFactory.h"

#include <Bandit.h>
#include <Devil.h>
#include <Engine.h>
#include <Frog.h>
#include <RiftBinder.h>
#include <WalkingDead.h>
#include <Witch.h>

namespace Nawia::Game {

	std::shared_ptr<Entity::Entity> BossEnemyFactory::create(
		const std::string& type,
		const std::string& name,
		const int max_hp,
		Core::Engine* engine)
	{
		if (!engine)
			return nullptr;

		auto player = engine->getPlayer();
		auto* map = engine->getCurrentMap();

		if (type == "Devil") {
			return std::shared_ptr<Entity::Entity>(Entity::DevilBuilder()
				.setName(name).setMap(map).setMaxHp(max_hp)
				.setTarget(player).setAudioManager(&engine->getAudioManager())
				.build());
		}

		if (type == "Witch") {
			return std::shared_ptr<Entity::Entity>(Entity::WitchBuilder()
				.setName(name).setMap(map).setMaxHp(max_hp)
				.setTarget(player).setAudioManager(&engine->getAudioManager())
				.build());
		}

		if (type == "WalkingDead") {
			return std::shared_ptr<Entity::Entity>(Entity::WalkingDeadBuilder()
				.setName(name).setMap(map).setMaxHp(max_hp)
				.setTarget(player).setAudioManager(&engine->getAudioManager())
				.build());
		}

		if (type == "Bandit") {
			auto bandit = Entity::BanditBuilder()
				.setName(name).setMap(map).setMaxHp(max_hp)
				.setTarget(player).setAudioManager(&engine->getAudioManager())
				.build();
			bandit->ensureKnifeThrowAbility(&engine->getResourceManager());
			return std::shared_ptr<Entity::Entity>(std::move(bandit));
		}

		if (type == "Frog") {
			return std::shared_ptr<Entity::Entity>(Entity::FrogBuilder()
				.setName(name).setMap(map).setEngine(engine).setMaxHp(max_hp)
				.setTarget(player).setAudioManager(&engine->getAudioManager())
				.build());
		}

		if (type == "RiftBinder" || type == "rift_binder" || type == "Dragon" || type == "dragon") {
			return std::shared_ptr<Entity::Entity>(Entity::RiftBinderBuilder()
				.setName(name).setMap(map).setMaxHp(max_hp)
				.setTarget(player).setAudioManager(&engine->getAudioManager())
				.build());
		}

		return nullptr;
	}

} // namespace Nawia::Game
