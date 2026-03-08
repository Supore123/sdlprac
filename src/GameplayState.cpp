#include "GameplayState.h"
#include "EngineContext.h"
#include "Camera.h"
#include "Renderer2D.h"
#include "InputManager.h"
#include "GameStateMachine.h"
#include "Components.h"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <iostream>
#include <algorithm>
#include <cmath>     // FIX: std::abs(float) — not guaranteed by <algorithm>

// -------------------------------------------------------------------------- //
//  Level layout — 25 columns × 15 rows, tileSize = 48px = 1200 × 720        //
//  0 = air   1 = stone   2 = dirt   4 = grass-top                           //
// -------------------------------------------------------------------------- //
// clang-format off
static const std::vector<int> k_levelTiles = {
//  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // row  0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // row  1
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // row  2
    0, 0, 0, 0, 0, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 0,  // row  3  high platforms
    0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0,  // row  4
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // row  5
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0,  // row  6  mid platform
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,  // row  7
    0, 0, 0, 4, 4, 4, 0, 0, 0, 0, 4, 4, 4, 0, 0, 0, 0, 0, 4, 4, 4, 4, 0, 0, 0,  // row  8  staggered low platforms
    0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0,  // row  9
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // row 10
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // row 11
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // row 12
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,  // row 13  ground surface
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  // row 14  dirt sub-layer
};
// clang-format on

static constexpr int   k_cols     = 25;
static constexpr int   k_rows     = 15;
static constexpr int   k_tileSize = 48;


// -------------------------------------------------------------------------- //
//  Construction                                                               //
// -------------------------------------------------------------------------- //

GameplayState::GameplayState(EngineContext* ctx)
    : m_ctx(ctx)
{}

// -------------------------------------------------------------------------- //
//  Lifecycle                                                                  //
// -------------------------------------------------------------------------- //

void GameplayState::onEnter()
{
    std::cout << "[GameplayState] Enter\n";

    m_world.clear();
    buildLevel();
    spawnPlayer();



    // Fade in from black
    m_fadeAlpha  = 1.f;
    m_fadingIn   = true;
    m_fadingOut  = false;
}

void GameplayState::onExit()
{
    std::cout << "[GameplayState] Exit\n";
    m_world.clear();
}

// -------------------------------------------------------------------------- //
//  Event                                                                      //
// -------------------------------------------------------------------------- //

void GameplayState::handleEvent(SDL_Event& /*e*/)
{
    // Input is read via InputManager in update() — no direct event handling needed.
}

// -------------------------------------------------------------------------- //
//  Update                                                                     //
// -------------------------------------------------------------------------- //

void GameplayState::update(float dt)
{
    // ── Fade in / out ────────────────────────────────────────────────────────
    if (m_fadingIn)
    {
        m_fadeAlpha -= m_fadeSpeed * dt;
        if (m_fadeAlpha <= 0.f)
        {
            m_fadeAlpha = 0.f;
            m_fadingIn  = false;
        }
    }

    if (m_fadingOut)
    {
        m_fadeAlpha += m_fadeSpeed * dt;
        if (m_fadeAlpha >= 1.f)
        {
            m_fadeAlpha = 1.f;
            m_ctx->gsm->pop();   // triggers onExit + returns to SplashState
            return;
        }
    }

    // ── ESC — begin fade-out back to previous state ──────────────────────────
    if (!m_fadingOut && m_ctx->input->isActionPressed("back"))
    {
        m_fadingOut = true;
        return;
    }

    // ── Player input ─────────────────────────────────────────────────────────
    processPlayerInput(dt);

    // ── Physics ──────────────────────────────────────────────────────────────
    m_physics.update(m_world, m_tilemap, dt);


    // ── Animation ────────────────────────────────────────────────────────────
    updateAnimatorState();
    m_animSystem.update(m_world, dt);
}

// -------------------------------------------------------------------------- //
//  Render                                                                     //
// -------------------------------------------------------------------------- //

void GameplayState::render()
{
    glClearColor(0.36f, 0.61f, 0.85f, 1.f);   // sky blue
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // ── World (tilemap + entities) ────────────────────────────────────────────
    Renderer2D& r   = *m_ctx->renderer;
    Camera&     cam = *m_ctx->camera;

    r.begin(cam);
        m_tilemap.draw(r);
        renderEntities();
    r.end();

    // ── HUD (screen-space) ───────────────────────────────────────────────────
    if (m_player != NULL_ENTITY && m_world.has<PlayerComponent>(m_player))
    {
        auto& pc = m_world.get<PlayerComponent>(m_player);
        m_hud.begin(r, cam);
            m_hud.drawPanel({ 10.f, 10.f }, { 180.f, 44.f });
            m_hud.drawHealthBar({ 20.f, 18.f }, pc.health, pc.maxHealth, 26.f, 8.f);
        m_hud.end();
    }

    // ── Fade overlay (drawn last so it covers everything) ────────────────────
    renderFadeOverlay();
}

// -------------------------------------------------------------------------- //
//  Private — level / entity setup                                            //
// -------------------------------------------------------------------------- //

void GameplayState::buildLevel()
{
    m_tilemap.load(k_cols, k_rows, k_tileSize, k_levelTiles);

}

void GameplayState::spawnPlayer()
{
    // Start the player just above the ground (row 13), near the left side
    float startX = 3.f  * k_tileSize;
    float startY = 12.f * k_tileSize;   // one tile above the ground row

    m_player = m_world.create();

    m_world.add<TagComponent>(m_player, { "player" });

    m_world.add<TransformComponent>(m_player, {
        .position = { startX, startY },
        .scale    = { 1.f, 1.f }
    });

    // Sprite: orange-red coloured quad (no texture needed for the demo)
    m_world.add<SpriteComponent>(m_player, {
        .textureID = 0,           // 0 → falls through to white texture
        .size      = { 36.f, 48.f },
        .tint      = { 0.95f, 0.50f, 0.15f, 1.f },   // bright orange
        .uvMin     = { 0.f, 0.f },
        .uvMax     = { 1.f, 1.f },
        .layer     = 1
    });

    m_world.add<RigidbodyComponent>(m_player, {
        .velocity     = { 0.f, 0.f },
        .gravityScale = 1.f,
        .useGravity   = true
    });

    // Collider slightly inset horizontally so the player slides through gaps
    m_world.add<BoxColliderComponent>(m_player, {
        .offset = { 4.f, 0.f },
        .size   = { 28.f, 48.f }
    });

    m_world.add<PlayerComponent>(m_player);

    // ── Animation clips (UV-based, works with any spritesheet) ───────────────
    //
    // These clips use a single-frame UV (the whole texture) since we're
    // rendering plain colour quads. Swap frame UVs here once you add a
    // real spritesheet with multiple columns.
    //
    // Example for a 4-frame run animation on a 4×1 sheet:
    //   AnimationFrame f0 = { {0.00f, 0.f}, {0.25f, 1.f} };
    //   AnimationFrame f1 = { {0.25f, 0.f}, {0.50f, 1.f} };
    //   ...

}

// -------------------------------------------------------------------------- //
//  Private — per-frame logic                                                  //
// -------------------------------------------------------------------------- //


void GameplayState::updateAnimatorState()
{
    if (m_player == NULL_ENTITY) return;
    if (!m_world.has<AnimatorComponent>(m_player)) return;

    auto& rb   = m_world.get<RigidbodyComponent>(m_player);
    auto& anim = m_world.get<AnimatorComponent>(m_player);

    if (!rb.onGround)
        AnimationSystem::play(anim, "jump");
    else if (std::abs(rb.velocity.x) > 10.f)
        AnimationSystem::play(anim, "run");
    else
        AnimationSystem::play(anim, "idle");
}

void GameplayState::renderEntities()
{
    Renderer2D& r = *m_ctx->renderer;

    // Draw all entities that have Transform + Sprite, sorted by layer
    auto entities = m_world.view<TransformComponent, SpriteComponent>();

    // Simple insertion sort by layer (entity count is tiny in a platformer)
    std::stable_sort(entities.begin(), entities.end(),
        [this](Entity a, Entity b) {
            return m_world.get<SpriteComponent>(a).layer
                 < m_world.get<SpriteComponent>(b).layer;
        });

    for (Entity e : entities)
    {
        auto& tf = m_world.get<TransformComponent>(e);
        auto& sp = m_world.get<SpriteComponent>(e);

        // Apply scale (handles left/right flip via negative scale.x)
        glm::vec2 scaledSize = {
            sp.size.x * std::abs(tf.scale.x),
            sp.size.y * std::abs(tf.scale.y)
        };

        // Offset position so flipped sprites don't shift origin
        glm::vec2 drawPos = tf.position;
        if (tf.scale.x < 0.f)
            drawPos.x += sp.size.x;   // compensate for flipped origin

        if (sp.textureID != 0)
        {
            r.drawQuad(drawPos, scaledSize, sp.textureID,
                       sp.tint, tf.rotation, sp.uvMin, sp.uvMax);
        }
        else
        {
            r.drawQuad(drawPos, scaledSize, sp.tint, tf.rotation);
        }
    }
}

void GameplayState::renderFadeOverlay()
{
    if (m_fadeAlpha <= 0.f) return;

    Renderer2D& r   = *m_ctx->renderer;
    Camera&     cam = *m_ctx->camera;

    r.begin(cam);
        r.drawQuad({ 0.f, 0.f },
                   { cam.getViewProjection()[0][0] == 0.f
                       ? 1280.f : 1280.f,   // always full screen
                     720.f },
                   glm::vec4(0.f, 0.f, 0.f, m_fadeAlpha));
    r.end();
}
    // Load the tileset and pass it to the tilemap so draw() uses UV-mapped quads
    // instead of the solid-colour fallback.
    // Sheet layout: 8 columns × 4 rows  (each tile = 1/8 wide, 1/4 tall in UV space).
    // Expected file: res/tiles.png  — replace path/dims to match your actual asset.
    auto tex = m_ctx->resources->loadTexture("tiles", "res/tiles.png", /*pixelated=*/true);
    if (tex.isValid())
        m_tilemap.setTilesetTexture(tex.id, /*tilesetCols=*/8, /*tilesetRows=*/4);
    else
        std::cout << "[GameplayState] tiles.png not found — using coloured quad fallback.\n";
    // Layout assumed: 4 columns × 3 rows spritesheet
    //   Row 0 (y: 0.00–0.33): idle  — 2 frames
    //   Row 1 (y: 0.33–0.67): run   — 4 frames
    //   Row 2 (y: 0.67–1.00): jump  — 1 frame
    //
    // When no spritesheet is available the UVs all resolve to the white 1×1
    // texture, so the tinted quad fallback still works correctly.

    auto makeFrame = [](int col, int row, int totalCols, int totalRows) -> AnimationFrame {
        float fw = 1.f / static_cast<float>(totalCols);
        float fh = 1.f / static_cast<float>(totalRows);
        return AnimationFrame {
            { col * fw,       row * fh       },
            { (col + 1) * fw, (row + 1) * fh }
        };
    };

    constexpr int kCols = 4, kRows = 3;

    AnimationClip idle;
    idle.name   = "idle";
    idle.fps    = 4.f;
    idle.loop   = true;
    idle.frames = { makeFrame(0, 0, kCols, kRows),
                    makeFrame(1, 0, kCols, kRows) };

    AnimationClip run;
    run.name   = "run";
    run.fps    = 10.f;
    run.loop   = true;
    run.frames = { makeFrame(0, 1, kCols, kRows),
                   makeFrame(1, 1, kCols, kRows),
                   makeFrame(2, 1, kCols, kRows),
                   makeFrame(3, 1, kCols, kRows) };

    AnimationClip jump;
    jump.name   = "jump";
    jump.fps    = 4.f;
    jump.loop   = false;
    jump.frames = { makeFrame(0, 2, kCols, kRows) };

    AnimatorComponent anim;
    anim.clips["idle"] = idle;
    anim.clips["run"]  = run;
    anim.clips["jump"] = jump;
    anim.currentClip   = "idle";
    m_world.add<AnimatorComponent>(m_player, std::move(anim));
// Tune these two constants to taste:
static constexpr float k_coyoteTime    = 0.12f;   // seconds of post-ledge grace
static constexpr float k_jumpBufferTime = 0.10f;  // seconds of pre-land buffer
void GameplayState::processPlayerInput(float dt)
{
    if (m_player == NULL_ENTITY) return;

    auto& rb = m_world.get<RigidbodyComponent>(m_player);
    auto& pc = m_world.get<PlayerComponent>(m_player);

    // ── Coyote time ──────────────────────────────────────────────────────────
    // Grace window: player can still jump for a short time after walking off a ledge.
    if (rb.onGround)
        m_coyoteTimer = k_coyoteTime;
    else if (m_coyoteTimer > 0.f)
        m_coyoteTimer -= dt;

    // ── Jump buffer ──────────────────────────────────────────────────────────
    // Pre-input: if the player presses jump slightly before landing, still jump.
    if (m_ctx->input->isActionPressed("confirm"))
        m_jumpBufferTimer = k_jumpBufferTime;
    else if (m_jumpBufferTimer > 0.f)
        m_jumpBufferTimer -= dt;

    // ── Horizontal movement ───────────────────────────────────────────────────
    float targetVX = 0.f;
    if (m_ctx->input->isActionDown("left"))
    {
        targetVX       = -pc.speed;
        pc.facingRight = false;
    }
    if (m_ctx->input->isActionDown("right"))
    {
        targetVX       = pc.speed;
        pc.facingRight = true;
    }
    rb.velocity.x = targetVX;

    // ── Jump — coyote time + buffer combined ──────────────────────────────────
    bool canJump   = (m_coyoteTimer > 0.f);
    bool wantsJump = (m_jumpBufferTimer > 0.f);
    if (canJump && wantsJump)
    {
        rb.velocity.y     = -pc.jumpForce;
        m_coyoteTimer     = 0.f;
        m_jumpBufferTimer = 0.f;
        m_ctx->resources->playSound("jump");
    }

    // Land detection: was airborne last frame, now on ground.
    if (rb.onGround && !m_wasOnGround)
        m_ctx->resources->playSound("land");
    m_wasOnGround = rb.onGround;

    // Flip sprite scale to face the direction of travel
    auto& tf   = m_world.get<TransformComponent>(m_player);
    tf.scale.x = pc.facingRight ? 1.f : -1.f;

    (void)dt;
}
    m_camOffset = { 0.f, 0.f };
    m_ctx->camera->setPosition({ 0.f, 0.f, 0.f });
    // Lerp camera position to keep the player centred on screen.
    if (m_player != NULL_ENTITY && m_world.has<TransformComponent>(m_player))
    {
        auto& tf = m_world.get<TransformComponent>(m_player);

        // Target: centre the player sprite on screen
        constexpr float k_screenW  = 1280.f;
        constexpr float k_screenH  = 720.f;
        constexpr float k_lerpSpeed = 6.f;   // higher = snappier follow

        float targetX = tf.position.x - k_screenW * 0.5f;
        float targetY = tf.position.y - k_screenH * 0.5f;

        // Clamp so the camera doesn't scroll past level edges
        float levelW = static_cast<float>(k_cols * k_tileSize);
        float levelH = static_cast<float>(k_rows * k_tileSize);
        targetX = std::clamp(targetX, 0.f, std::max(0.f, levelW - k_screenW));
        targetY = std::clamp(targetY, 0.f, std::max(0.f, levelH - k_screenH));

        // Lerp current camera offset toward target
        m_camOffset.x += (targetX - m_camOffset.x) * k_lerpSpeed * dt;
        m_camOffset.y += (targetY - m_camOffset.y) * k_lerpSpeed * dt;

        // Push into the camera as a position offset — the ortho projection stays
        // fixed at (0,0)→(1280,720); we shift the view matrix instead.
        glm::vec3 camPos = m_ctx->camera->getPosition();
        camPos.x = m_camOffset.x;
        camPos.y = m_camOffset.y;
        m_ctx->camera->setPosition(camPos);
    }
