#include "AbilityEffect.h"
#include "AbilityStats.h"

namespace Nawia::Entity {

	class Projectile : public AbilityEffect {
	public:
		Projectile(const std::string& name, float x, float y, float target_x, float target_y, const std::shared_ptr<Texture2D> &tex, const AbilityStats& stats, Entity* caster);

		void update(float dt) override;

		[[nodiscard]] bool checkCollision(const std::shared_ptr<Entity> &target) const override;
		void onCollision(const std::shared_ptr<Entity> &target) override;

	private:
		float _speed;
		float _vel_x, _vel_y;
		Entity* _caster;
	};

} // namespace Nawia::Entity