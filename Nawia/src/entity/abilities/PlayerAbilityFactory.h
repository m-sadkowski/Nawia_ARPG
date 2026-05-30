#pragma once

#include <Ability.h>
#include <FireballAbility.h>
#include <UnarmedMeleeAbility.h>

#include <json.hpp>

#include <memory>
#include <string>
#include <vector>

namespace Nawia::Core { class ResourceManager; }

namespace Nawia::Entity::PlayerAbilityFactory {

	[[nodiscard]] const nlohmann::json& getPlayerSetupConfig();

	[[nodiscard]] std::shared_ptr<UnarmedMeleeAbility> createUnarmedAbility(
		const nlohmann::json& ability_json,
		Core::ResourceManager& resource_manager);

	[[nodiscard]] std::vector<std::shared_ptr<UnarmedMeleeAbility>> createUnarmedAbilities(
		const nlohmann::json& setup_json,
		Core::ResourceManager& resource_manager);

	[[nodiscard]] std::shared_ptr<UnarmedMeleeAbility> createUnarmedAbilityByName(
		const nlohmann::json& setup_json,
		Core::ResourceManager& resource_manager,
		const std::string& ability_name);

	[[nodiscard]] std::shared_ptr<FireballAbility> createStarterFireball(
		const nlohmann::json& setup_json,
		Core::ResourceManager& resource_manager);

} // namespace Nawia::Entity::PlayerAbilityFactory
