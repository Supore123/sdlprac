



#include "PhysicsWorld.h"
#include "ECS.h"
#include "Components.h"
#include "Tilemap.h"

#include <algorithm>
#include <cmath>

// -------------------------------------------------------------------------- //
//  Public update                                                              //
// -------------------------------------------------------------------------- //

void PhysicsWorld::update(World& world, const Tilemap& tilemap, float dt)
{
    auto entities = world.view<TransformComponent,
                               RigidbodyComponent,
                               BoxColliderComponent>();

    for (Entity e : entities)
    {
        auto& tf = world.get<TransformComponent>(e);
        auto& rb = world.get<RigidbodyComponent>(e);
        auto& bc = world.get<BoxColliderComponent>(e);

        if (!rb.useGravity) continue;

        // ── 1. Gravity ───────────────────────────────────────────────────────
        rb.velocity.y += GRAVITY * rb.gravityScale * dt;

        // Terminal velocity cap (prevents tunnelling at very high speeds)
        rb.velocity.y = std::min(rb.velocity.y, 1200.f);

        // ── 2. X integration + collision ─────────────────────────────────────
        tf.position.x += rb.velocity.x * dt;
        if (!bc.isTrigger)
            resolveX(tf.position, bc.offset, bc.size, rb.velocity.x, tilemap);

        // ── 3. Y integration + collision ─────────────────────────────────────
        rb.onGround = false;
        tf.position.y += rb.velocity.y * dt;
        if (!bc.isTrigger)
            resolveY(tf.position, bc.offset, bc.size, rb.velocity.y,
                     rb.onGround, tilemap);
    }

    // ── Trigger overlap detection ─────────────────────────────────────────────
    if (onOverlap)
    {
        auto triggers = world.view<TransformComponent, BoxColliderComponent>();

        for (size_t i = 0; i < triggers.size(); ++i)
        {
            Entity a  = triggers[i];
            auto& tfa = world.get<TransformComponent>(a);
            auto& bca = world.get<BoxColliderComponent>(a);
            if (!bca.isTrigger) continue;

            glm::vec2 aPos  = tfa.position + bca.offset;

            for (size_t j = i + 1; j < triggers.size(); ++j)
            {
                Entity b  = triggers[j];
                auto& tfb = world.get<TransformComponent>(b);
                auto& bcb = world.get<BoxColliderComponent>(b);

                glm::vec2 bPos = tfb.position + bcb.offset;
                glm::vec2 pen;
                if (aabbOverlap(aPos, bca.size, bPos, bcb.size, pen))
                    onOverlap(a, b);
            }
        }
    }
}

// -------------------------------------------------------------------------- //
//  Private helpers                                                            //
// -------------------------------------------------------------------------- //

/**
 * Horizontal tile collision.
 * Moves the collider, then for every solid tile it overlaps pushes it out
 * along X only.
 */
void PhysicsWorld::resolveX(glm::vec2&       position,
                             const glm::vec2& offset,
                             const glm::vec2& size,
                             float&           velocityX,
                             const Tilemap&   tm) const
{
    float ts = static_cast<float>(tm.tileSize());

    // AABB world corners after integration
    float left   = position.x + offset.x;
    float right  = left + size.x;
    float top    = position.y + offset.y;
    float bottom = top + size.y - 1.f;

    int tileMinX = static_cast<int>(std::floor(left   / ts));
    int tileMaxX = static_cast<int>(std::floor(right  / ts));
    int tileMinY = static_cast<int>(std::floor(top    / ts));
    int tileMaxY = static_cast<int>(std::floor(bottom / ts));

    for (int ty = tileMinY; ty <= tileMaxY; ++ty)
    {
        for (int tx = tileMinX; tx <= tileMaxX; ++tx)
        {
            if (!tm.isSolid(tx, ty)) continue;

            float tileLeft  = static_cast<float>(tx) * ts;
            float tileRight = tileLeft + ts;

            // push out along X only
            if (velocityX > 0.f)
            {
                // moving right — align right edge with tile left face
                position.x = tileLeft - offset.x - size.x;
            }
            else if (velocityX < 0.f)
            {
                // moving left — align left edge with tile right face
                position.x = tileRight - offset.x;
            }
            velocityX = 0.f;

            // recompute AABB for any further tiles on this row
            left  = position.x + offset.x;
            right = left + size.x;
        }
    }
}

/**
 * Vertical tile collision.
 * Moves the collider, then resolves solid tile overlaps along Y.
 * Sets onGround = true when landing on a tile from above.
 */
void PhysicsWorld::resolveY(glm::vec2&       position,
                             const glm::vec2& offset,
                             const glm::vec2& size,
                             float&           velocityY,
                             bool&            onGround,
                             const Tilemap&   tm) const
{
    float ts = static_cast<float>(tm.tileSize());

    float left   = position.x + offset.x;
    float right  = left + size.x - 1.f;
    float top    = position.y + offset.y;
    float bottom = top + size.y;

    int tileMinX = static_cast<int>(std::floor(left   / ts));
    int tileMaxX = static_cast<int>(std::floor(right  / ts));
    int tileMinY = static_cast<int>(std::floor(top    / ts));
    int tileMaxY = static_cast<int>(std::floor(bottom / ts));

    for (int ty = tileMinY; ty <= tileMaxY; ++ty)
    {
        for (int tx = tileMinX; tx <= tileMaxX; ++tx)
        {
            if (!tm.isSolid(tx, ty)) continue;

            float tileTop    = static_cast<float>(ty) * ts;
            float tileBottom = tileTop + ts;

            if (velocityY > 0.f)
            {
                // Falling — land on top of tile
                position.y = tileTop - offset.y - size.y;
                onGround   = true;
            }
            else if (velocityY < 0.f)
            {
                // Jumping — hit ceiling
                position.y = tileBottom - offset.y;
            }

            velocityY = 0.f;

            // Recompute top/bottom for subsequent checks
            top    = position.y + offset.y;
            bottom = top + size.y;
        }
    }
}

// -------------------------------------------------------------------------- //
//  Static utility                                                             //
// -------------------------------------------------------------------------- //

bool PhysicsWorld::aabbOverlap(glm::vec2 aPos, glm::vec2 aSize,
                                glm::vec2 bPos, glm::vec2 bSize,
                                glm::vec2& penetration)
{
    float aRight  = aPos.x + aSize.x;
    float aBottom = aPos.y + aSize.y;
    float bRight  = bPos.x + bSize.x;
    float bBottom = bPos.y + bSize.y;

    if (aPos.x >= bRight  || bPos.x >= aRight  ||
        aPos.y >= bBottom || bPos.y >= aBottom)
        return false;

    float overlapX = std::min(aRight,  bRight)  - std::max(aPos.x, bPos.x);
    float overlapY = std::min(aBottom, bBottom) - std::max(aPos.y, bPos.y);

    penetration = (overlapX < overlapY)
        ? glm::vec2(overlapX * (aPos.x < bPos.x ? -1.f : 1.f), 0.f)
        : glm::vec2(0.f, overlapY * (aPos.y < bPos.y ? -1.f : 1.f));

    return true;
}

