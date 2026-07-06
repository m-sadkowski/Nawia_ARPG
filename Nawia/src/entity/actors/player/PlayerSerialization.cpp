#include "Player.h"
#include "PlayerInternal.h"

#include <ItemDatabase.h>

#include <algorithm>

namespace Nawia::Entity {

	nlohmann::json Player::serializeProfile() const {
		return {
			{"level", _level},
			{"exp", _exp},
			{"exp_to_next_level", _exp_to_next_lvl},
			{"gold", _gold},
			{"food_count", _food_count},
			{"fireball_unlocked", _fireball_unlocked},
			{"base_stats", PlayerDetail::statsToJson(_base_stats)},
			{"inventory", _backpack ? _backpack->serialize() : nlohmann::json::object()},
			{"equipment", _equipment ? _equipment->serialize() : nlohmann::json::array()}
		};
	}

	void Player::applyProfile(const nlohmann::json& data, Item::ItemDatabase& item_database) {
		clearItems();

		if (!data.is_object())
			return;

		_level = data.value("level", _level);
		_exp = data.value("exp", _exp);
		_exp_to_next_lvl = data.value("exp_to_next_level", _exp_to_next_lvl);
		_gold = data.value("gold", _gold);
		_food_count = data.value("food_count", _food_count);
		_fireball_unlocked = data.value("fireball_unlocked", _fireball_unlocked);
		_base_stats = PlayerDetail::statsFromJson(data.value("base_stats", nlohmann::json::object()), _base_stats);

		if (data.contains("inventory") && _backpack)
			_backpack->applyJson(data["inventory"], item_database);

		if (data.contains("equipment") && data["equipment"].is_array()) {
			for (const auto& entry : data["equipment"]) {
				const int item_id = entry.value("item_id", 0);
				if (item_id <= 0)
					continue;

				if (auto item = item_database.createItem(item_id))
					equipItem(item);
			}
		}

		recalculateStats();
		ensureUnlockedFireballAbility();
	}

	nlohmann::json Player::serializeLocationView() const {
		const int safe_hp = isDead() ? std::max(1, getMaxHP() / 2) : std::clamp(getHP(), 1, getMaxHP());
		return {
			{"position", PlayerDetail::vector2ToJson(getPosition())},
			{"altitude", getAltitude()},
			{"hp", safe_hp},
			{"max_hp", getMaxHP()},
			{"respawn_point", PlayerDetail::vector2ToJson(_respawn_point)}
		};
	}

	void Player::applyLocationView(const nlohmann::json& data) {
		if (!data.is_object())
			return;

		const Vector2 position = PlayerDetail::vector2FromJson(data.value("position", nlohmann::json::object()), getPosition());
		setX(position.x);
		setY(position.y);
		setAltitude(data.value("altitude", getAltitude()));
		_respawn_point = PlayerDetail::vector2FromJson(data.value("respawn_point", nlohmann::json::object()), _respawn_point);

		setHP(data.value("hp", getHP()));
		stop();
	}

} // namespace Nawia::Entity
