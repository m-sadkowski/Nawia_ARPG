#include "PlayerAbilityFactory.h"

#include <Logger.h>
#include <ResourceManager.h>

#include <fstream>

namespace Nawia::Entity::PlayerAbilityFactory {

	namespace {
		constexpr const char* PLAYER_SETUP_PATH = "assets/data/player_setup.json";

		nlohmann::json loadJsonDocument(const std::string& path) {
			std::ifstream file(path);
			if (!file.is_open()) {
				Core::Logger::errorLog("PlayerAbilityFactory: nie mozna otworzyc JSON: " + path);
				return {};
			}

			nlohmann::json data;
			try {
				file >> data;
			} catch (const nlohmann::json::parse_error&) {
				Core::Logger::errorLog("PlayerAbilityFactory: blad parsowania JSON: " + path);
				return {};
			}

			return data;
		}

		AbilityTargetType parseAbilityTargetType(const std::string& value) {
			if (value == "UNIT")
				return AbilityTargetType::UNIT;
			if (value == "SELF")
				return AbilityTargetType::SELF;
			return AbilityTargetType::POINT;
		}

		UnarmedMeleeEffect::Shape parseUnarmedShape(const std::string& value) {
			if (value == "ForwardRectangle")
				return UnarmedMeleeEffect::Shape::ForwardRectangle;
			return UnarmedMeleeEffect::Shape::Cone;
		}
	}

	const nlohmann::json& getPlayerSetupConfig() {
		static const nlohmann::json config = loadJsonDocument(PLAYER_SETUP_PATH);
		return config;
	}

	std::shared_ptr<UnarmedMeleeAbility> createUnarmedAbility(
		const nlohmann::json& ability_json,
		Core::ResourceManager& resource_manager)
	{
		if (!ability_json.is_object())
			return nullptr;

		const std::string name = ability_json.value("name", "Unarmed");
		const auto icon = resource_manager.getTexture(ability_json.value("icon", ""));
		return std::make_shared<UnarmedMeleeAbility>(
			name,
			ability_json.value("stats_key", name),
			ability_json.value("animation", "Idle_Loop"),
			parseAbilityTargetType(ability_json.value("target_type", "POINT")),
			ability_json.value("direct_target_hit", false),
			parseUnarmedShape(ability_json.value("shape", "Cone")),
			ability_json.value("spawn_ratio", 0.45f),
			ability_json.value("hitbox_width", 1.0f),
			ability_json.value("knockback_distance", 0.0f),
			ability_json.value("ping_pong_animation", true),
			icon);
	}

	std::vector<std::shared_ptr<UnarmedMeleeAbility>> createUnarmedAbilities(
		const nlohmann::json& setup_json,
		Core::ResourceManager& resource_manager)
	{
		std::vector<std::shared_ptr<UnarmedMeleeAbility>> abilities;
		const auto abilities_it = setup_json.find("unarmed_abilities");
		if (abilities_it == setup_json.end() || !abilities_it->is_array())
			return abilities;

		for (const auto& ability_json : *abilities_it) {
			if (auto ability = createUnarmedAbility(ability_json, resource_manager))
				abilities.push_back(std::move(ability));
		}

		return abilities;
	}

	std::shared_ptr<UnarmedMeleeAbility> createUnarmedAbilityByName(
		const nlohmann::json& setup_json,
		Core::ResourceManager& resource_manager,
		const std::string& ability_name)
	{
		const auto abilities_it = setup_json.find("unarmed_abilities");
		if (abilities_it == setup_json.end() || !abilities_it->is_array())
			return nullptr;

		for (const auto& ability_json : *abilities_it) {
			if (ability_json.value("name", "") == ability_name)
				return createUnarmedAbility(ability_json, resource_manager);
		}

		return nullptr;
	}

} // namespace Nawia::Entity::PlayerAbilityFactory
