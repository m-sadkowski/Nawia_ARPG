#pragma once

#include <raylib.h>

namespace Nawia::Entity {

	class Entity;

	/**
	 * @enum ColliderType
	 * @brief Typ geometrii używany przy testach kolizji.
	 */
	enum class ColliderType {
		NONE,
		CIRCLE,
		RECTANGLE,
		CONE
	};

	/**
	 * @class Collider
	 * @brief Bazowy interfejs geometrii kolizyjnej przypiętej do encji.
	 */
	class Collider {
	public:
		/**
		 * @brief Tworzy kolider z właścicielem i opcjonalnym przesunięciem.
		 */
		Collider(Entity* owner, float offset_x = 0.0f, float offset_y = 0.0f)
			: _owner(owner),
			  _offset(Vector2{offset_x, offset_y}) {}

		virtual ~Collider() = default;

		/** @brief Zwraca typ geometrii kolidera. */
		[[nodiscard]] virtual ColliderType getType() const = 0;

		/** @brief Sprawdza kolizję z innym koliderem. */
		[[nodiscard]] virtual bool checkCollision(const Collider* other) const = 0;

		/** @brief Sprawdza kolizję z pudełkiem ograniczającym modelu. */
		[[nodiscard]] virtual bool checkCollision(const BoundingBox& target_box) const = 0;

		/** @brief Sprawdza dokładniejszą kolizję z siatką modelu encji. */
		[[nodiscard]] virtual bool checkMeshCollision(const Entity* target) const = 0;

		/** @brief Sprawdza trafienie punktem ekranu po projekcji przez kamerę. */
		[[nodiscard]] virtual bool checkPoint(float screen_x, float screen_y, const Camera3D& camera) const = 0;

		/** @brief Renderuje kolider w trybie diagnostycznym. */
		virtual void render(const Camera3D& camera) const = 0;

		/** @brief Ustawia lokalne przesunięcie względem właściciela. */
		void setOffset(float x, float y) { _offset = Vector2{x, y}; }

		/** @brief Zwraca pozycję kolidera w świecie. */
		[[nodiscard]] Vector2 getPosition() const;

	protected:
		Entity* _owner = nullptr;
		Vector2 _offset = {0.0f, 0.0f};
	};

	/**
	 * @class CircleCollider
	 * @brief Okrągły kolider dobry dla postaci i prostych efektów.
	 */
	class CircleCollider : public Collider {
	public:
		/** @brief Tworzy kolider okręgu z promieniem i przesunięciem. */
		CircleCollider(Entity* owner, float radius, float offset_x = 0.0f, float offset_y = 0.0f)
			: Collider(owner, offset_x, offset_y),
			  _radius(radius) {}

		[[nodiscard]] ColliderType getType() const override { return ColliderType::CIRCLE; }
		[[nodiscard]] bool checkCollision(const Collider* other) const override;
		[[nodiscard]] bool checkCollision(const BoundingBox& target_box) const override;
		[[nodiscard]] bool checkMeshCollision(const Entity* target) const override;
		[[nodiscard]] bool checkPoint(float screen_x, float screen_y, const Camera3D& camera) const override;
		void render(const Camera3D& camera) const override;

		/** @brief Zwraca promień okręgu. */
		[[nodiscard]] float getRadius() const { return _radius; }

	private:
		float _radius = 0.0f;
	};

	/**
	 * @class RectangleCollider
	 * @brief Prostokątny kolider używany dla triggerów i obiektów statycznych.
	 */
	class RectangleCollider : public Collider {
	public:
		/** @brief Tworzy kolider prostokąta z wymiarami i przesunięciem. */
		RectangleCollider(Entity* owner, float width, float height, float offset_x = 0.0f, float offset_y = 0.0f)
			: Collider(owner, offset_x, offset_y),
			  _width(width),
			  _height(height) {}

		[[nodiscard]] ColliderType getType() const override { return ColliderType::RECTANGLE; }
		[[nodiscard]] bool checkCollision(const Collider* other) const override;
		[[nodiscard]] bool checkCollision(const BoundingBox& target_box) const override;
		[[nodiscard]] bool checkMeshCollision(const Entity* target) const override;
		[[nodiscard]] bool checkPoint(float screen_x, float screen_y, const Camera3D& camera) const override;
		void render(const Camera3D& camera) const override;

		/** @brief Zwraca szerokość prostokąta. */
		[[nodiscard]] float getWidth() const { return _width; }

		/** @brief Zwraca wysokość prostokąta. */
		[[nodiscard]] float getHeight() const { return _height; }

		/** @brief Zwraca prostokąt w przestrzeni świata 2D. */
		[[nodiscard]] Rectangle getRect() const;

	private:
		float _width = 0.0f;
		float _height = 0.0f;
	};

	/**
	 * @class ConeCollider
	 * @brief Stożek trafienia używany przez ataki kierunkowe.
	 */
	class ConeCollider : public Collider {
	public:
		/** @brief Tworzy kolider stożka z promieniem, kątem i przesunięciem. */
		ConeCollider(Entity* owner, float radius, float angle, float offset_x = 0.0f, float offset_y = 0.0f)
			: Collider(owner, offset_x, offset_y),
			  _radius(radius),
			  _angle(angle) {}

		[[nodiscard]] ColliderType getType() const override { return ColliderType::CONE; }
		[[nodiscard]] bool checkCollision(const Collider* other) const override;
		[[nodiscard]] bool checkCollision(const BoundingBox& target_box) const override;
		[[nodiscard]] bool checkMeshCollision(const Entity* target) const override;
		[[nodiscard]] bool checkPoint(float screen_x, float screen_y, const Camera3D& camera) const override;
		void render(const Camera3D& camera) const override;

		/** @brief Zwraca promień stożka. */
		[[nodiscard]] float getRadius() const { return _radius; }

		/** @brief Zwraca pełny kąt stożka w stopniach. */
		[[nodiscard]] float getAngle() const { return _angle; }

	private:
		float _radius = 0.0f;
		float _angle = 0.0f;
	};

} // namespace Nawia::Entity
