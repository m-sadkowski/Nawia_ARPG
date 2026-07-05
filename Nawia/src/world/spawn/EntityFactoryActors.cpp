#include "EntityFactory.h"
#include "EntityFactoryCommon.h"

#include <Bandit.h>
#include <Devil.h>
#include <Engine.h>
#include <Friend.h>
#include <Frog.h>
#include <MiniMushroomInfected.h>
#include <RiftBinder.h>
#include <Spider.h>
#include <WalkingDead.h>
#include <Witch.h>
#include <Worm.h>

#include <SwordSlashAbility.h>

namespace Nawia::World {

	std::shared_ptr<Entity::Entity> EntityFactory::createDevil(
		const nlohmann::json& data, const SpawnContext& context)
	{
		const auto basics = EntityFactoryDetail::readBasics(data, "Devil", 100);
		auto* engine = context.engine;
		auto player = engine->getPlayer();

		auto devil = Entity::DevilBuilder()
			.setName(basics.name)
			.setPosition(basics.position)
			.setMap(context.map)
			.setMaxHp(basics.hp)
			.setTarget(player)
			.setAudioManager(&engine->getAudioManager())
			.build();

		return devil;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createRiftBinder(
		const nlohmann::json& data, const SpawnContext& context)
	{
		const auto basics = EntityFactoryDetail::readBasics(data, "Siewca Chaosu", 420);
		auto* engine = context.engine;

		auto boss = Entity::RiftBinderBuilder()
			.setName(basics.name)
			.setPosition(basics.position)
			.setMap(context.map)
			.setMaxHp(basics.hp)
			.setTarget(engine ? engine->getPlayer() : nullptr)
			.setAudioManager(engine ? &engine->getAudioManager() : nullptr)
			.build();

		return boss;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createWitch(
		const nlohmann::json& data, const SpawnContext& context)
	{
		const auto basics = EntityFactoryDetail::readBasics(data, "Czarownica", 160);
		auto* engine = context.engine;

		auto witch = Entity::WitchBuilder()
			.setName(basics.name)
			.setPosition(basics.position)
			.setMap(context.map)
			.setMaxHp(basics.hp)
			.setTarget(engine ? engine->getPlayer() : nullptr)
			.setAudioManager(engine ? &engine->getAudioManager() : nullptr)
			.build();

		return witch;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createBandit(
		const nlohmann::json& data, const SpawnContext& context)
	{
		const auto basics = EntityFactoryDetail::readBasics(data, "Bandyta", 80);
		auto* engine = context.engine;
		auto player = engine->getPlayer();

		auto bandit = Entity::BanditBuilder()
			.setName(basics.name)
			.setPosition(basics.position)
			.setMap(context.map)
			.setMaxHp(basics.hp)
			.setTarget(player)
			.setAudioManager(&engine->getAudioManager())
			.build();

		bandit->ensureKnifeThrowAbility(&engine->getResourceManager());

		return bandit;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createWalkingDead(
		const nlohmann::json& data, const SpawnContext& context)
	{
		const auto basics = EntityFactoryDetail::readBasics(data, "Walking Dead", 80);
		auto* engine = context.engine;
		auto player = engine->getPlayer();

		auto wd = Entity::WalkingDeadBuilder()
			.setName(basics.name)
			.setPosition(basics.position)
			.setMap(context.map)
			.setMaxHp(basics.hp)
			.setTarget(player)
			.setAudioManager(&engine->getAudioManager())
			.build();

		return wd;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createFrog(
		const nlohmann::json& data, const SpawnContext& context)
	{
		const auto basics = EntityFactoryDetail::readBasics(data, "Ropuch", 95);
		auto* engine = context.engine;

		auto frog = std::shared_ptr<Entity::Frog>(Entity::FrogBuilder()
			.setName(basics.name)
			.setPosition(basics.position)
			.setMap(context.map)
			.setEngine(engine)
			.setMaxHp(basics.hp)
			.setTarget(engine->getPlayer())
			.setAudioManager(&engine->getAudioManager())
			.build());
		return frog;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createWorm(
		const nlohmann::json& data, const SpawnContext& context)
	{
		const auto basics = EntityFactoryDetail::readBasics(data, "Robal", 35);
		auto* engine = context.engine;

		auto worm = std::shared_ptr<Entity::Worm>(Entity::WormBuilder()
			.setName(basics.name)
			.setPosition(basics.position)
			.setMap(context.map)
			.setMaxHp(basics.hp)
			.setTarget(engine->getPlayer())
			.setAudioManager(&engine->getAudioManager())
			.build());
		return worm;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createSpider(
		const nlohmann::json& data, const SpawnContext& context)
	{
		const auto basics = EntityFactoryDetail::readBasics(data, "Pajak", 240);
		auto* engine = context.engine;

		auto spider = std::shared_ptr<Entity::Spider>(Entity::SpiderBuilder()
			.setName(basics.name)
			.setPosition(basics.position)
			.setMap(context.map)
			.setMaxHp(basics.hp)
			.setTarget(engine ? engine->getPlayer() : nullptr)
			.setAudioManager(engine ? &engine->getAudioManager() : nullptr)
			.build());
		return spider;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createMiniMushroomInfected(
		const nlohmann::json& data, const SpawnContext& context)
	{
		const auto basics = EntityFactoryDetail::readBasics(data, "Zly Gzibek", 45);
		auto* engine = context.engine;

		auto mushroom = std::shared_ptr<Entity::MiniMushroomInfected>(Entity::MiniMushroomInfectedBuilder()
			.setName(basics.name)
			.setPosition(basics.position)
			.setMap(context.map)
			.setMaxHp(basics.hp)
			.setTarget(engine->getPlayer())
			.setAudioManager(&engine->getAudioManager())
			.build());
		return mushroom;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createFriend(
		const nlohmann::json& data, const SpawnContext& context)
	{
		const auto basics = EntityFactoryDetail::readBasics(data, "Friend", 100);
		auto* engine = context.engine;

		auto friend_entity = Entity::FriendBuilder()
			.setName(basics.name)
			.setPosition(basics.position)
			.setMap(context.map)
			.setMaxHp(basics.hp)
			.setAudioManager(&engine->getAudioManager())
			.build();

		auto& rm = engine->getResourceManager();
		const auto sword_slash_icon = rm.getTexture("assets/textures/icons/sword_slash_icon.png");
		friend_entity->addAbility(std::make_shared<Entity::SwordSlashAbility>(nullptr, sword_slash_icon));

		return friend_entity;
	}

} // namespace Nawia::World
