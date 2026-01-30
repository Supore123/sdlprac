#pragma once


#include <glm/glm.hpp>
#include <functional>

class World;
class Tilemap;

/**
 * PhysicsWorld
 *
 * Resolves platformer physics for every entity that has
 * TransformComponent + RigidbodyComponent + BoxColliderComponent.
 *
 * ── Per-frame call ───────────────────────────────────────────────────────────
 *   physicsWorld.update(ecsWorld, tilemap, dt);
 *
 * ── Algorithm (per entity) ──────────────────────────────────────────────────
 *   1. Apply gravity to velocity.y
 *   2. Move along X by velocity.x * dt → resolve X tile collisions.
 *   3. Move along Y by velocity.y * dt → resolve Y tile collisions.
 *      Collision from above sets onGround = true.
 *
 * Separating the X and Y passes is the standard trick that makes corner
 * handling and wall-sliding feel correct in a platformer.
 *
 * ── Overlap callbacks ────────────────────────────────────────────────────────
 *   physicsWorld.onOverlap = [](Entity a, Entity b) { ... };
 *   Called once per pair per frame for trigger colliders.
 */
class PhysicsWorld
{
public:
    // Pixels per second² — roughly 1 g at ~32 px/m world scale
    static constexpr float GRAVITY = 980.f;

    // Called for each trigger-collider overlap this frame
    using OverlapCallback = std::function<void(uint32_t entityA, uint32_t entityB)>;
    OverlapCallback onOverlap;

    /**
     * Advance physics one timestep.
     * @param world    ECS world containing Transform + Rigidbody + Collider
     * @param tilemap  Solid tile grid used for static collision geometry
     * @param dt       Delta time in seconds
     */
    void update(World& world, const Tilemap& tilemap, float dt);

    // ── Utility ──────────────────────────────────────────────────────────────

    /**
     * AABB overlap test.
     * @param aPos / aSize   First box top-left corner + dimensions
     * @param bPos / bSize   Second box
     * Returns true on overlap; fills 'penetration' with the minimum
     * separation vector (pointing from b into a).
     */
    static bool aabbOverlap(glm::vec2 aPos, glm::vec2 aSize,
                            glm::vec2 bPos, glm::vec2 bSize,
                            glm::vec2& penetration);

private:
    void resolveX(glm::vec2& position, const glm::vec2& colliderOffset,
                  const glm::vec2& colliderSize, float& velocityX,
                  const Tilemap& tilemap) const;

    void resolveY(glm::vec2& position, const glm::vec2& colliderOffset,
                  const glm::vec2& colliderSize, float& velocityY,
                  bool& onGround, const Tilemap& tilemap) const;
};
