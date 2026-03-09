// START_COMMIT_1
#include "AttackSystem.h"
#include "PhysicsWorld.h"
#include "AnimationSystem.h"

#include <glm/glm.hpp>
#include <cmath>
#include <vector>
#include <iostream>
// END_COMMIT_1

// -------------------------------------------------------------------------- //
//  registerClips — adds the "attack" animation clip to the player            //
// -------------------------------------------------------------------------- //

// START_COMMIT_2
void AttackSystem::registerClips(World& world, Entity player, CharacterType type)
{
    if (!world.has<AnimatorComponent>(player)) return;
    auto& anim = world.get<AnimatorComponent>(player);

    // All three sheets now have 4 rows: idle(0), run(1), jump(2), attack(3)
    auto makeFrame = [](int col, int row) -> AnimationFrame {
        float fw = 1.f / 4.f;
        float fh = 1.f / 4.f;   // 4 rows now
        return { { col*fw, row*fh }, { (col+1)*fw, (row+1)*fh } };
    };

    // Also fix idle/run/jump UVs to use 4-row sheet (fh = 0.25 not 0.333)
    AnimationClip idle;
    idle.name="idle"; idle.fps=3.f; idle.loop=true;
    idle.frames = { makeFrame(0,0), makeFrame(1,0) };

    AnimationClip run;
    run.name="run"; run.fps=10.f; run.loop=true;
    run.frames = { makeFrame(0,1), makeFrame(1,1),
                   makeFrame(2,1), makeFrame(3,1) };

    AnimationClip jump;
    jump.name="jump"; jump.fps=4.f; jump.loop=false;
    jump.frames = { makeFrame(0,2) };

    AnimationClip attack;
    attack.name="attack"; attack.fps=12.f; attack.loop=false;

    switch (type)
    {
        case CharacterType::Knight:
            // 2-frame sword swing
            attack.frames = { makeFrame(0,3), makeFrame(1,3) };
            break;
        case CharacterType::Ninja:
            // 2-frame throw pose
            attack.frames = { makeFrame(0,3), makeFrame(1,3) };
            break;
        case CharacterType::Robot:
            // 1-frame fire pose held for duration
            attack.frames = { makeFrame(0,3) };
            break;
    }

    anim.clips["idle"]   = idle;
    anim.clips["run"]    = run;
    anim.clips["jump"]   = jump;
    anim.clips["attack"] = attack;
}
// END_COMMIT_2

// -------------------------------------------------------------------------- //
//  update                                                                     //
// -------------------------------------------------------------------------- //

// START_COMMIT_3
void AttackSystem::update(World& world, Entity player,
                          bool attackPressed, float dt)
{
    if (player == NULL_ENTITY || !world.has<AttackComponent>(player)) return;

    auto& ac = world.get<AttackComponent>(player);

    // Tick cooldown
    if (ac.attackCooldown > 0.f)
        ac.attackCooldown -= dt;

    // Tick active attack
    if (ac.attacking)
    {
        ac.attackTimer += dt;
        ac.fxTimer     += dt;
        ac.fxFrame      = static_cast<int>(ac.fxTimer / 0.07f);

        // Knight: check melee hits every frame during attack window
        if (ac.character == CharacterType::Knight)
            checkMeleeHits(world, player);

        if (ac.attackTimer >= ac.attackDuration)
        {
            ac.attacking          = false;
            ac.attackTimer        = 0.f;
            ac.fxTimer            = 0.f;
            ac.fxFrame            = 0;
            ac.projectileFired    = false;
            ac.attackCooldown     = ac.cooldownDuration;

            // Return to idle animation
            if (world.has<AnimatorComponent>(player))
                AnimationSystem::play(world.get<AnimatorComponent>(player), "idle");
        }
    }

    // Start new attack
    if (attackPressed && !ac.attacking && ac.attackCooldown <= 0.f)
    {
        ac.attacking       = true;
        ac.attackTimer     = 0.f;
        ac.fxTimer         = 0.f;
        ac.fxFrame         = 0;
        ac.projectileFired = false;

        if (world.has<AnimatorComponent>(player))
            AnimationSystem::play(world.get<AnimatorComponent>(player), "attack");

        // Ninja and Robot fire projectile at attack start
        if (ac.character == CharacterType::Ninja ||
            ac.character == CharacterType::Robot)
        {
            fireProjectile(world, player, ac.character,
                           m_fxShurikenTex, m_fxLaserTex);
            ac.projectileFired = true;
        }
    }

    // Tick player projectiles
    updateProjectiles(world, dt);
}
// END_COMMIT_3

// -------------------------------------------------------------------------- //
//  render — draws FX overlays                                                //
// -------------------------------------------------------------------------- //

// START_COMMIT_4
void AttackSystem::render(World& world, Entity player,
                          Renderer2D& renderer, const Camera& cam,
                          GLuint fxSwordTex, GLuint fxHitTex,
                          GLuint fxShurikenTex, GLuint fxLaserTex)
{
    // Cache tex IDs for use in update()
    m_fxSwordTex    = fxSwordTex;
    m_fxHitTex      = fxHitTex;
    m_fxShurikenTex = fxShurikenTex;
    m_fxLaserTex    = fxLaserTex;

    if (player == NULL_ENTITY || !world.has<AttackComponent>(player)) return;
    auto& ac = world.get<AttackComponent>(player);
    if (!ac.attacking) return;
    if (!world.has<TransformComponent>(player)) return;

    auto& tf       = world.get<TransformComponent>(player);
    bool  facingR  = (tf.scale.x >= 0.f);
    float flipSign = facingR ? 1.f : -1.f;

    renderer.begin(cam);

    if (ac.character == CharacterType::Knight && fxSwordTex)
    {
        // Draw swing arc FX offset to the right of the player
        int    frame   = std::min(ac.fxFrame, 3);
        float  fw      = 1.f / 4.f;
        glm::vec2 uvMin = { frame * fw, 0.f };
        glm::vec2 uvMax = { (frame+1) * fw, 1.f };
        glm::vec2 fxPos = tf.position + glm::vec2{ facingR ? 24.f : -72.f, -8.f };
        glm::vec2 fxSize = { 64.f * flipSign, 64.f };
        renderer.drawQuad(fxPos, { std::abs(fxSize.x), fxSize.y },
                          fxSwordTex, { 1.f,1.f,1.f,0.9f }, 0.f, uvMin, uvMax);
    }

    renderer.end();
}
// END_COMMIT_4

// -------------------------------------------------------------------------- //
//  Private — melee hit detection                                             //
// -------------------------------------------------------------------------- //

// START_COMMIT_5
void AttackSystem::checkMeleeHits(World& world, Entity player)
{
    if (!world.has<AttackComponent>(player)) return;
    if (!world.has<TransformComponent>(player)) return;

    auto& ac = world.get<AttackComponent>(player);
    auto& tf = world.get<TransformComponent>(player);

    bool facingR = (tf.scale.x >= 0.f);

    // Build melee hitbox in world space
    glm::vec2 hitPos = tf.position + glm::vec2{
        facingR ? ac.meleeOffset.x : -(ac.meleeOffset.x + ac.meleeSize.x),
        ac.meleeOffset.y
    };

    auto enemies = world.view<EnemyComponent, TransformComponent, BoxColliderComponent>();
    for (Entity e : enemies)
    {
        auto& ec  = world.get<EnemyComponent>(e);
        if (!ec.alive) continue;

        auto& etf = world.get<TransformComponent>(e);
        auto& ebc = world.get<BoxColliderComponent>(e);

        glm::vec2 ePos = etf.position + ebc.offset;
        glm::vec2 pen;

        if (PhysicsWorld::aabbOverlap(hitPos, ac.meleeSize, ePos, ebc.size, pen))
        {
            ec.health -= 1.f;
            ec.hitFlashTimer = 0.15f;
            if (ec.health <= 0.f)
                ec.alive = false;

            if (onMeleeHit)
                onMeleeHit(player, e);
        }
    }
}

// -------------------------------------------------------------------------- //
//  Private — spawn projectile                                                //
// -------------------------------------------------------------------------- //

void AttackSystem::fireProjectile(World& world, Entity player,
                                  CharacterType type,
                                  GLuint shurikenTex, GLuint laserTex)
{
    if (!world.has<TransformComponent>(player)) return;
    auto& tf      = world.get<TransformComponent>(player);
    bool  facingR = (tf.scale.x >= 0.f);
    float dir     = facingR ? 1.f : -1.f;

    Entity proj = world.create();
    world.add<TagComponent>(proj, { "player_projectile" });

    bool isSword = (type == CharacterType::Knight); // unused here but kept for clarity
    (void)isSword;

    bool  isLaser   = (type == CharacterType::Robot);
    GLuint texID    = isLaser ? laserTex : shurikenTex;
    glm::vec2 size  = isLaser ? glm::vec2{32.f, 12.f} : glm::vec2{16.f, 16.f};
    float speed     = isLaser ? 550.f : 380.f;

    glm::vec2 spawnPos = tf.position + glm::vec2{ facingR ? 44.f : -size.x, 16.f };

    world.add<TransformComponent>(proj, { .position=spawnPos, .scale={1.f,1.f} });
    world.add<SpriteComponent>(proj, {
        .textureID = texID,
        .size      = size,
        .tint      = { 1.f,1.f,1.f,1.f },
        .uvMin     = { 0.f,0.f },
        .uvMax     = { 0.25f, 1.f },
        .layer     = 2
    });
    world.add<BoxColliderComponent>(proj, {
        .offset    = { 2.f, 2.f },
        .size      = { size.x-4.f, size.y-4.f },
        .isTrigger = true
    });
    world.add<PlayerProjectileComponent>(proj, {
        .velocity = { dir * speed, 0.f },
        .lifetime = 1.6f,
        .damage   = 1.f
    });

    // Spinning animation for shuriken
    if (!isLaser && shurikenTex)
    {
        AnimationClip spin;
        spin.name="spin"; spin.fps=16.f; spin.loop=true;
        float fw = 1.f/4.f;
        for (int c=0;c<4;++c)
            spin.frames.push_back({{c*fw,0.f},{(c+1)*fw,1.f}});
        AnimatorComponent anim;
        anim.clips["spin"] = spin;
        anim.currentClip   = "spin";
        world.add<AnimatorComponent>(proj, std::move(anim));
    }
}

// -------------------------------------------------------------------------- //
//  Private — tick player projectiles                                         //
// -------------------------------------------------------------------------- //

void AttackSystem::updateProjectiles(World& world, float dt)
{
    auto projs = world.view<PlayerProjectileComponent, TransformComponent>();
    std::vector<Entity> toDestroy;

    for (Entity p : projs)
    {
        auto& pc = world.get<PlayerProjectileComponent>(p);
        auto& tf = world.get<TransformComponent>(p);

        pc.lifetime -= dt;
        tf.position += pc.velocity * dt;

        if (pc.lifetime <= 0.f)
        {
            toDestroy.push_back(p);
            continue;
        }

        // Check vs enemies
        if (!world.has<BoxColliderComponent>(p)) continue;
        auto& pbc = world.get<BoxColliderComponent>(p);

        auto enemies = world.view<EnemyComponent, TransformComponent, BoxColliderComponent>();
        for (Entity e : enemies)
        {
            auto& ec = world.get<EnemyComponent>(e);
            if (!ec.alive) continue;

            auto& etf = world.get<TransformComponent>(e);
            auto& ebc = world.get<BoxColliderComponent>(e);
            glm::vec2 pen;

            if (PhysicsWorld::aabbOverlap(
                    tf.position + pbc.offset, pbc.size,
                    etf.position + ebc.offset, ebc.size, pen))
            {
                ec.health -= pc.damage;
                ec.hitFlashTimer = 0.15f;
                if (ec.health <= 0.f)
                    ec.alive = false;

                if (onProjectileHit)
                    onProjectileHit(p, e);

                toDestroy.push_back(p);
                break;
            }
        }
    }

    for (Entity e : toDestroy)
        if (world.has<PlayerProjectileComponent>(e))  // guard double-destroy
            world.destroy(e);
}
// END_COMMIT_5
