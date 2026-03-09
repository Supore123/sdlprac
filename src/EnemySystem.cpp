// START_COMMIT_1
#include "EnemySystem.h"
#include "Components.h"
#include "Tilemap.h"
#include "AnimationSystem.h"
#include "PhysicsWorld.h"

#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>
#include <iostream>
// END_COMMIT_1

// -------------------------------------------------------------------------- //
//  Public update — dispatches to per-type handlers                           //
// -------------------------------------------------------------------------- //

// START_COMMIT_2
void EnemySystem::update(World& world, const Tilemap& tilemap,
                         Entity playerEntity, float dt)
{
    // Grab player position once
    glm::vec2 playerPos = { -9999.f, -9999.f };
    bool playerAlive = (playerEntity != NULL_ENTITY
                        && world.has<TransformComponent>(playerEntity));
    if (playerAlive)
        playerPos = world.get<TransformComponent>(playerEntity).position;

    // Tick projectiles first (they don't need enemy data)
    updateProjectiles(world, playerEntity, dt);

    // Process death: add DeathComponent for newly dead enemies, destroy when timer expires
    std::vector<Entity> toDestroy;
    for (Entity e : world.view<EnemyComponent, TransformComponent>())
    {
        auto& ec = world.get<EnemyComponent>(e);
        if (!ec.alive)
        {
            if (!world.has<DeathComponent>(e))
            {
                world.add<DeathComponent>(e, DeathComponent{});
                if (onEnemyDeath && world.has<TransformComponent>(e))
                    onEnemyDeath(world, e, world.get<TransformComponent>(e).position);
            }
            else
            {
                auto& dc = world.get<DeathComponent>(e);
                dc.timer -= dt;
                if (dc.timer <= 0.f)
                    toDestroy.push_back(e);
            }
        }
    }
    for (Entity e : toDestroy)
        world.destroy(e);

    // Update each enemy
    auto enemies = world.view<EnemyComponent, TransformComponent, SpriteComponent>();
    for (Entity e : enemies)
    {
        auto& ec = world.get<EnemyComponent>(e);
        if (!ec.alive) continue;

        if (ec.hitFlashTimer > 0.f)
            ec.hitFlashTimer -= dt;

        auto& tf = world.get<TransformComponent>(e);
        // SpriteComponent read by renderEntities — not needed here

        // Flip sprite to match facing
        tf.scale.x = ec.facingRight ? 1.f : -1.f;

        switch (ec.type)
        {
            case EnemyType::Patroller:
                if (world.has<RigidbodyComponent>(e))
                    updatePatroller(world, e, ec, tf,
                                    world.get<RigidbodyComponent>(e),
                                    tilemap, dt);
                break;

            case EnemyType::Flyer:
                updateFlyer(world, e, ec, tf, playerPos, dt);
                break;

            case EnemyType::Shooter:
                updateShooter(world, e, ec, tf, playerPos, dt);
                break;

            case EnemyType::Jumper:
                if (world.has<RigidbodyComponent>(e))
                    updateJumper(world, e, ec, tf,
                                 world.get<RigidbodyComponent>(e),
                                 playerPos, dt);
                break;
        }

        // Contact damage — simple AABB vs player
        if (playerAlive && onEnemyContact
            && world.has<BoxColliderComponent>(e)
            && world.has<BoxColliderComponent>(playerEntity))
        {
            auto& ebc  = world.get<BoxColliderComponent>(e);
            auto& pbc  = world.get<BoxColliderComponent>(playerEntity);
            auto& ptf  = world.get<TransformComponent>(playerEntity);

            glm::vec2 ePos = tf.position      + ebc.offset;
            glm::vec2 pPos = ptf.position     + pbc.offset;
            glm::vec2 pen;
            if (PhysicsWorld::aabbOverlap(ePos, ebc.size, pPos, pbc.size, pen))
                onEnemyContact(e, playerEntity);
        }
    }
}
// END_COMMIT_2

// -------------------------------------------------------------------------- //
//  Spawn helpers                                                              //
// -------------------------------------------------------------------------- //

// START_COMMIT_3
Entity EnemySystem::spawnPatroller(World& world, glm::vec2 pos,
                                   float patrolLeft, float patrolRight,
                                   GLuint texID)
{
    Entity e = world.create();

    world.add<TagComponent>(e, { "enemy_patroller" });
    world.add<TransformComponent>(e, { .position = pos, .scale = {1.f,1.f} });
    world.add<SpriteComponent>(e, {
        .textureID = texID,
        .size      = { 48.f, 48.f },
        .tint      = { 1.f,1.f,1.f,1.f },
        .uvMin     = { 0.f, 0.f },
        .uvMax     = { 0.25f, 1.f },   // frame 0 of 4-col sheet
        .layer     = 1
    });
    world.add<RigidbodyComponent>(e, {
        .velocity     = { 60.f, 0.f },
        .gravityScale = 1.f,
        .useGravity   = true
    });
    world.add<BoxColliderComponent>(e, { .offset={4.f,4.f}, .size={40.f,40.f} });

    EnemyComponent ec;
    ec.type        = EnemyType::Patroller;
    ec.speed       = 60.f;
    ec.patrolLeft  = patrolLeft;
    ec.patrolRight = patrolRight;
    ec.facingRight = true;
    world.add<EnemyComponent>(e, ec);

    // Animation: 4-frame walk loop (cols 0-3, single row)
    auto makeUV = [](int col) -> AnimationFrame {
        float w = 1.f/4.f;
        return { {col*w, 0.f}, {(col+1)*w, 1.f} };
    };
    AnimationClip walk;
    walk.name="walk"; walk.fps=6.f; walk.loop=true;
    walk.frames = { makeUV(0), makeUV(1), makeUV(2), makeUV(3) };
    AnimatorComponent anim;
    anim.clips["walk"] = walk;
    anim.currentClip   = "walk";
    world.add<AnimatorComponent>(e, std::move(anim));

    return e;
}

Entity EnemySystem::spawnFlyer(World& world, glm::vec2 pos,
                               float hoverY, GLuint texID)
{
    Entity e = world.create();

    world.add<TagComponent>(e, { "enemy_flyer" });
    world.add<TransformComponent>(e, { .position = pos, .scale = {1.f,1.f} });
    world.add<SpriteComponent>(e, {
        .textureID = texID,
        .size      = { 48.f, 48.f },
        .tint      = { 1.f,1.f,1.f,1.f },
        .uvMin     = { 0.f, 0.f },
        .uvMax     = { 0.25f, 1.f },
        .layer     = 1
    });
    world.add<BoxColliderComponent>(e, { .offset={8.f,12.f}, .size={32.f,28.f} });

    EnemyComponent ec;
    ec.type       = EnemyType::Flyer;
    ec.speed      = 80.f;
    ec.patrolLeft = hoverY;   // reuse patrolLeft as hoverY
    world.add<EnemyComponent>(e, ec);

    auto makeUV = [](int col) -> AnimationFrame {
        float w = 1.f/4.f;
        return { {col*w, 0.f}, {(col+1)*w, 1.f} };
    };
    AnimationClip flap;
    flap.name="flap"; flap.fps=8.f; flap.loop=true;
    flap.frames = { makeUV(0), makeUV(1), makeUV(2), makeUV(3) };
    AnimatorComponent anim;
    anim.clips["flap"] = flap;
    anim.currentClip   = "flap";
    world.add<AnimatorComponent>(e, std::move(anim));

    return e;
}

Entity EnemySystem::spawnShooter(World& world, glm::vec2 pos,
                                 GLuint texID, GLuint projTexID)
{
    Entity e = world.create();

    world.add<TagComponent>(e, { "enemy_shooter" });
    world.add<TransformComponent>(e, { .position = pos, .scale = {1.f,1.f} });
    world.add<SpriteComponent>(e, {
        .textureID = texID,
        .size      = { 48.f, 48.f },
        .tint      = { 1.f,1.f,1.f,1.f },
        .uvMin     = { 0.f, 0.f },
        .uvMax     = { 0.25f, 1.f },
        .layer     = 1
    });
    world.add<BoxColliderComponent>(e, { .offset={8.f,8.f}, .size={32.f,38.f} });

    // Store projTexID so updateShooter can fire projectiles with it.
    // spawnShooter is static so we pass projTexID back via the EnemyComponent
    // by repurposing patrolRight (unused for shooters) as a float cast.
    EnemyComponent ec;
    ec.type        = EnemyType::Shooter;
    ec.speed       = 0.f;
    ec.timer       = 2.f;
    ec.patrolRight = static_cast<float>(projTexID);  // store texID here
    world.add<EnemyComponent>(e, ec);

    // Bob animation
    auto makeUV = [](int col) -> AnimationFrame {
        float w = 1.f/4.f;
        return { {col*w, 0.f}, {(col+1)*w, 1.f} };
    };
    AnimationClip bob;
    bob.name="bob"; bob.fps=3.f; bob.loop=true;
    bob.frames = { makeUV(0), makeUV(1), makeUV(2), makeUV(1) };
    AnimationClip shoot_clip;
    shoot_clip.name="shoot"; shoot_clip.fps=6.f; shoot_clip.loop=false;
    shoot_clip.frames = { makeUV(3), makeUV(3) };
    AnimatorComponent anim;
    anim.clips["bob"]   = bob;
    anim.clips["shoot"] = shoot_clip;
    anim.currentClip    = "bob";
    world.add<AnimatorComponent>(e, std::move(anim));

    return e;
}

Entity EnemySystem::spawnJumper(World& world, glm::vec2 pos, GLuint texID)
{
    Entity e = world.create();

    world.add<TagComponent>(e, { "enemy_jumper" });
    world.add<TransformComponent>(e, { .position = pos, .scale = {1.f,1.f} });
    world.add<SpriteComponent>(e, {
        .textureID = texID,
        .size      = { 48.f, 48.f },
        .tint      = { 1.f,1.f,1.f,1.f },
        .uvMin     = { 0.f, 0.f },
        .uvMax     = { 0.25f, 1.f },
        .layer     = 1
    });
    world.add<RigidbodyComponent>(e, {
        .velocity     = { 0.f, 0.f },
        .gravityScale = 1.2f,
        .useGravity   = true
    });
    world.add<BoxColliderComponent>(e, { .offset={6.f,6.f}, .size={36.f,38.f} });

    EnemyComponent ec;
    ec.type  = EnemyType::Jumper;
    ec.speed = 200.f;
    ec.timer = 1.5f;   // time until next jump
    world.add<EnemyComponent>(e, ec);

    auto makeUV = [](int col) -> AnimationFrame {
        float w = 1.f/4.f;
        return { {col*w, 0.f}, {(col+1)*w, 1.f} };
    };
    AnimationClip crouch_clip;
    crouch_clip.name="crouch"; crouch_clip.fps=4.f; crouch_clip.loop=false;
    crouch_clip.frames = { makeUV(0) };
    AnimationClip air_clip;
    air_clip.name="air"; air_clip.fps=4.f; air_clip.loop=true;
    air_clip.frames = { makeUV(1) };
    AnimationClip land_clip;
    land_clip.name="land"; land_clip.fps=4.f; land_clip.loop=false;
    land_clip.frames = { makeUV(3), makeUV(2) };
    AnimatorComponent anim;
    anim.clips["crouch"] = crouch_clip;
    anim.clips["air"]    = air_clip;
    anim.clips["land"]   = land_clip;
    anim.currentClip     = "crouch";
    world.add<AnimatorComponent>(e, std::move(anim));

    return e;
}
// END_COMMIT_3

// -------------------------------------------------------------------------- //
//  Per-type update implementations                                           //
// -------------------------------------------------------------------------- //

// START_COMMIT_4
void EnemySystem::updatePatroller(World& /*world*/, Entity /*e*/,
                                  EnemyComponent& ec,
                                  TransformComponent& tf,
                                  RigidbodyComponent& rb,
                                  const Tilemap& /*tilemap*/, float /*dt*/)
{
    // Reverse at patrol bounds
    if (tf.position.x <= ec.patrolLeft)
    {
        ec.facingRight  = true;
        rb.velocity.x   = ec.speed;
    }
    else if (tf.position.x + 48.f >= ec.patrolRight)
    {
        ec.facingRight  = false;
        rb.velocity.x   = -ec.speed;
    }

    // Ensure horizontal velocity is always set (physics may zero it on wall hit)
    if (rb.velocity.x == 0.f)
    {
        rb.velocity.x = ec.facingRight ? ec.speed : -ec.speed;
    }
}

void EnemySystem::updateFlyer(World& /*world*/, Entity /*e*/,
                               EnemyComponent& ec,
                               TransformComponent& tf,
                               glm::vec2 playerPos, float dt)
{
    float hoverY = ec.patrolLeft;   // stored in patrolLeft at spawn
    float dist   = glm::length(playerPos - tf.position);

    if (dist < ec.aggroRange)
    {
        // Chase player horizontally, maintain hover height
        glm::vec2 dir = glm::normalize(playerPos - tf.position);
        tf.position.x += dir.x * ec.speed * dt;
        // Smooth vertical return to hover height
        float targetY  = playerPos.y - 80.f;   // stay above player
        tf.position.y += (targetY - tf.position.y) * 3.f * dt;
        ec.facingRight = dir.x > 0.f;
    }
    else
    {
        // Idle bob at hover height
        ec.timer      += dt;
        tf.position.y  = hoverY + std::sin(ec.timer * 2.f) * 12.f;
    }
}

void EnemySystem::updateShooter(World& world, Entity e,
                                 EnemyComponent& ec,
                                 TransformComponent& tf,
                                 glm::vec2 playerPos, float dt)
{
    float dist = glm::length(playerPos - tf.position);
    ec.facingRight = (playerPos.x > tf.position.x);

    // Switch back to bob when shoot animation finishes
    if (world.has<AnimatorComponent>(e))
    {
        auto& anim = world.get<AnimatorComponent>(e);
        if (anim.currentClip == "shoot" && !anim.playing)
            AnimationSystem::play(anim, "bob");
    }

    // Idle bob
    ec.timer -= dt;

    if (dist < ec.aggroRange && ec.timer <= 0.f)
    {
        ec.timer = 2.5f;

        GLuint projTex = static_cast<GLuint>(ec.patrolRight);  // stored at spawn
        if (projTex != 0)
        {
            glm::vec2 dir = glm::normalize(playerPos - tf.position);

            Entity proj = world.create();
            world.add<TagComponent>(proj, { "projectile" });
            world.add<TransformComponent>(proj, {
                .position = tf.position + glm::vec2{24.f, 20.f},
                .scale    = {1.f,1.f}
            });
            world.add<SpriteComponent>(proj, {
                .textureID = projTex,
                .size      = { 16.f, 16.f },
                .tint      = { 1.f,1.f,1.f,1.f },
                .uvMin     = { 0.f, 0.f },
                .uvMax     = { 0.25f, 1.f },
                .layer     = 2
            });
            world.add<BoxColliderComponent>(proj, {
                .offset={2.f,2.f}, .size={12.f,12.f}, .isTrigger=true
            });
            world.add<ProjectileComponent>(proj, {
                .velocity  = dir * 260.f,
                .lifetime  = 2.5f,
                .fromEnemy = true
            });

            AnimationClip spin;
            spin.name="spin"; spin.fps=12.f; spin.loop=true;
            float w = 1.f/4.f;
            for (int c = 0; c < 4; ++c)
                spin.frames.push_back({{c*w, 0.f}, {(c+1)*w, 1.f}});
            AnimatorComponent anim;
            anim.clips["spin"] = spin;
            anim.currentClip   = "spin";
            world.add<AnimatorComponent>(proj, std::move(anim));
        }

        // Play shoot animation
        if (world.has<AnimatorComponent>(e))
            AnimationSystem::play(world.get<AnimatorComponent>(e), "shoot");
    }
}

void EnemySystem::updateJumper(World& world, Entity e,
                                EnemyComponent& ec,
                                TransformComponent& tf,
                                RigidbodyComponent& rb,
                                glm::vec2 playerPos, float dt)
{
    float dist = glm::length(playerPos - tf.position);

    ec.timer -= dt;

    // Switch animation based on grounded state
    if (world.has<AnimatorComponent>(e))
    {
        auto& anim = world.get<AnimatorComponent>(e);
        if (rb.onGround)
            AnimationSystem::play(anim, "crouch");
        else
            AnimationSystem::play(anim, "air");
    }

    if (rb.onGround && ec.timer <= 0.f && dist < ec.aggroRange)
    {
        // Jump toward the player
        glm::vec2 dir  = glm::normalize(playerPos - tf.position);
        rb.velocity.x  = dir.x * ec.speed;
        rb.velocity.y  = -420.f;   // jump force
        ec.facingRight = dir.x > 0.f;
        ec.timer       = 1.8f;
    }

    // Dampen horizontal velocity when on ground (no slide)
    if (rb.onGround)
        rb.velocity.x *= 0.7f;
}
// END_COMMIT_4

void EnemySystem::updateProjectiles(World& world, Entity playerEntity, float dt)
{
    auto projs = world.view<ProjectileComponent, TransformComponent>();
    std::vector<Entity> toDestroy;

    for (Entity p : projs)
    {
        auto& pc = world.get<ProjectileComponent>(p);
        auto& tf = world.get<TransformComponent>(p);

        pc.lifetime -= dt;
        tf.position  += pc.velocity * dt;

        if (pc.lifetime <= 0.f)
        {
            toDestroy.push_back(p);
            continue;
        }

        // Check vs player
        if (pc.fromEnemy && playerEntity != NULL_ENTITY
            && world.has<BoxColliderComponent>(p)
            && world.has<BoxColliderComponent>(playerEntity))
        {
            auto& pbc = world.get<BoxColliderComponent>(p);
            auto& plbc= world.get<BoxColliderComponent>(playerEntity);
            auto& pltf= world.get<TransformComponent>(playerEntity);
            glm::vec2 pen;
            if (PhysicsWorld::aabbOverlap(
                    tf.position + pbc.offset, pbc.size,
                    pltf.position + plbc.offset, plbc.size, pen))
            {
                if (onProjectileHit)
                    onProjectileHit(p, playerEntity);
                toDestroy.push_back(p);
            }
        }
    }

    for (Entity e : toDestroy)
        world.destroy(e);
}
