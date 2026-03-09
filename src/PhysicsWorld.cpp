



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

