#include "GameplayState.h"
#include "EngineContext.h"
#include "Camera.h"
#include "Renderer2D.h"
#include "InputManager.h"
#include "GameStateMachine.h"
#include "Components.h"
#include "EnemySystem.h"
#include "AttackSystem.h"
#include "AnimationSystem.h"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <random>
#include <vector>
#include <chrono>

// -------------------------------------------------------------------------- //
//  Tile IDs: 0=empty, 1=solid top (platform), 2=solid, 4=platform top       //
// -------------------------------------------------------------------------- //

static constexpr int   k_cols     = 50;
static constexpr int   k_rows     = 15;
static constexpr int   k_tileSize = 48;

static constexpr float k_coyoteTime     = 0.12f;
static constexpr float k_jumpBufferTime = 0.10f;

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

    m_score = 0;
    m_world.clear();
    buildLevel();
    spawnPlayer();

    // ── Attack system setup ──────────────────────────────────────────────────
    CharacterType charType = CharacterType::Knight;
    m_world.add<AttackComponent>(m_player, AttackComponent{ .character = charType });
    AttackSystem::registerClips(m_world, m_player, charType);

    m_fxSwordTex    = m_ctx->resources->loadTexture("fx_sword",    "res/fx_sword_swing.png", false).id;
    m_fxShurikenTex = m_ctx->resources->loadTexture("fx_shuriken", "res/fx_shuriken.png",    true).id;
    m_fxLaserTex    = m_ctx->resources->loadTexture("fx_laser",    "res/fx_laser.png",       false).id;
    m_fxHitTex      = m_ctx->resources->loadTexture("fx_hit",      "res/fx_hit.png",         false).id;
    m_fontTexID     = m_ctx->resources->loadTexture("font",        "res/font.png",            true).id;

    m_attackSystem.onMeleeHit = [](Entity /*player*/, Entity /*enemy*/) {};
    m_attackSystem.onProjectileHit = [](Entity /*proj*/, Entity /*enemy*/) {};

    // ── Input bindings ───────────────────────────────────────────────────────
    m_ctx->input->bindAction("left",        SDL_SCANCODE_A);
    m_ctx->input->bindAction("right",       SDL_SCANCODE_D);
    m_ctx->input->bindAction("jump",        SDL_SCANCODE_SPACE);
    m_ctx->input->bindAction("attack",      SDL_SCANCODE_J);
    m_ctx->input->bindAction("char_knight", SDL_SCANCODE_1);
    m_ctx->input->bindAction("char_ninja",  SDL_SCANCODE_2);
    m_ctx->input->bindAction("char_robot",  SDL_SCANCODE_3);
    m_ctx->input->bindAction("back",        SDL_SCANCODE_ESCAPE);

    // ── Audio ────────────────────────────────────────────────────────────────
    m_ctx->resources->loadSoundEffect("jump", "res/audio/jump.wav");
    m_ctx->resources->loadSoundEffect("land", "res/audio/land.wav");

    // ── Camera ───────────────────────────────────────────────────────────────
    m_camOffset = { 0.f, 0.f };
    m_ctx->camera->setPosition({ 0.f, 0.f, 0.f });

    m_fadeAlpha = 1.f;
    m_fadingIn  = true;
    m_fadingOut = false;
}

void GameplayState::onExit()
{
    std::cout << "[GameplayState] Exit\n";
    m_world.clear();
}

// -------------------------------------------------------------------------- //
//  Event                                                                      //
// -------------------------------------------------------------------------- //

void GameplayState::handleEvent(SDL_Event& /*e*/) {}

// -------------------------------------------------------------------------- //
//  Update                                                                     //
// -------------------------------------------------------------------------- //

void GameplayState::update(float dt)
{
    // ── Fade ─────────────────────────────────────────────────────────────────
    if (m_fadingIn)
    {
        m_fadeAlpha -= m_fadeSpeed * dt;
        if (m_fadeAlpha <= 0.f) { m_fadeAlpha = 0.f; m_fadingIn = false; }
    }
    if (m_fadingOut)
    {
        m_fadeAlpha += m_fadeSpeed * dt;
        if (m_fadeAlpha >= 1.f)
        {
            m_fadeAlpha = 1.f;
            m_ctx->gsm->pop();
            return;
        }
    }

    // ── ESC ──────────────────────────────────────────────────────────────────
    if (!m_fadingOut && m_ctx->input->isActionPressed("back"))
    {
        m_fadingOut = true;
        return;
    }

    // ── Character switching ───────────────────────────────────────────────────
    if (m_ctx->input->isActionPressed("char_knight"))
        switchCharacter(CharacterType::Knight);
    else if (m_ctx->input->isActionPressed("char_ninja"))
        switchCharacter(CharacterType::Ninja);
    else if (m_ctx->input->isActionPressed("char_robot"))
        switchCharacter(CharacterType::Robot);

    // ── Power-up timers ────────────────────────────────────────────────────────
    if (m_playerInvulnTimer > 0.f)  m_playerInvulnTimer  -= dt;
    if (m_speedBoostTimer > 0.f)    m_speedBoostTimer    -= dt;
    if (m_invincibilityTimer > 0.f) m_invincibilityTimer -= dt;

    // ── Player input + attack ────────────────────────────────────────────────
    processPlayerInput(dt);

    bool attackPressed = m_ctx->input->isActionPressed("attack");
    m_attackSystem.update(m_world, m_player, attackPressed, dt);

    // ── Physics ───────────────────────────────────────────────────────────────
    m_physics.update(m_world, m_tilemap, dt);

    // ── Enemies ───────────────────────────────────────────────────────────────
    m_enemySystem.update(m_world, m_tilemap, m_player, dt);

    // ── Camera ────────────────────────────────────────────────────────────────
    if (m_player != NULL_ENTITY && m_world.has<TransformComponent>(m_player))
    {
        auto& tf = m_world.get<TransformComponent>(m_player);

        // instead of hardcoded 1280×720 we read the ortho size from the camera
        float k_screenW   = m_ctx->camera->getOrthoWidth();
        float k_screenH   = m_ctx->camera->getOrthoHeight();
        constexpr float k_lerpSpeed = 6.f;

        float targetX = tf.position.x - k_screenW * 0.5f;
        float targetY = tf.position.y - k_screenH * 0.5f;

        float levelW = m_tilemap.worldWidth();
        float levelH = m_tilemap.worldHeight();
        targetX = std::clamp(targetX, 0.f, std::max(0.f, levelW - k_screenW));
        targetY = std::clamp(targetY, 0.f, std::max(0.f, levelH - k_screenH));

        m_camOffset.x += (targetX - m_camOffset.x) * k_lerpSpeed * dt;
        m_camOffset.y += (targetY - m_camOffset.y) * k_lerpSpeed * dt;

        glm::vec3 camPos = m_ctx->camera->getPosition();
        camPos.x = m_camOffset.x;
        camPos.y = m_camOffset.y;
        m_ctx->camera->setPosition(camPos);
    }

    // ── Animation ─────────────────────────────────────────────────────────────
    updateAnimatorState();
    m_animSystem.update(m_world, dt);
}

// -------------------------------------------------------------------------- //
//  Render                                                                     //
// -------------------------------------------------------------------------- //

void GameplayState::render()
{
    glClearColor(0.36f, 0.61f, 0.85f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Renderer2D& r   = *m_ctx->renderer;
    Camera&     cam = *m_ctx->camera;

    r.begin(cam);
        m_tilemap.draw(r);
        renderEntities();
    r.end();

    // ── Attack FX ─────────────────────────────────────────────────────────────
    m_attackSystem.render(m_world, m_player, r, cam,
                          m_fxSwordTex, m_fxHitTex,
                          m_fxShurikenTex, m_fxLaserTex);

    // ── HUD ───────────────────────────────────────────────────────────────────
    if (m_player != NULL_ENTITY && m_world.has<PlayerComponent>(m_player))
    {
        auto& pc = m_world.get<PlayerComponent>(m_player);
        m_hud.begin(r, cam);
            m_hud.drawPanel({ 10.f, 10.f }, { 180.f, 44.f });
            m_hud.drawHealthBar({ 20.f, 18.f }, pc.health, pc.maxHealth, 26.f, 8.f);
        m_hud.end();

        // Character label
        std::string charLabel = "1:KNIGHT";
        if (m_world.has<AttackComponent>(m_player))
            switch (m_world.get<AttackComponent>(m_player).character)
            {
                case CharacterType::Ninja: charLabel = "2:NINJA";  break;
                case CharacterType::Robot: charLabel = "3:ROBOT";  break;
                default: break;
            }
        m_hud.begin(r, cam);
            m_hud.drawText({ 10.f, 60.f }, "1:KNIGHT 2:NINJA 3:ROBOT",
                           m_fontTexID, 8.f, 8.f);
            m_hud.drawText({ 10.f, 72.f }, charLabel,
                           m_fontTexID, 10.f, 10.f, { 1.f, 1.f, 0.f, 1.f });
            // draw score on right edge
            std::string scoreText = "SCORE:" + std::to_string(m_score);
            m_hud.drawText({ 1100.f, 10.f }, scoreText,
                           m_fontTexID, 10.f, 10.f, {1,1,1,1});
        m_hud.end();
    }

    renderFadeOverlay();
}

// -------------------------------------------------------------------------- //
//  Procedural level generation — Mario-style                                 //
// -------------------------------------------------------------------------- //

struct ProceduralSpawns
{
    struct Platform { int col; int row; int width; };
    std::vector<Platform> patrollerPlatforms;
    std::vector<Platform> flyerPlatforms;   // row = hover height
    std::vector<Platform> shooterPlatforms;
    std::vector<Platform> jumperPlatforms;
    std::vector<std::pair<int, int>> coinTiles;
};

static std::pair<std::vector<int>, ProceduralSpawns> makeLevel(int cols, int rows, unsigned seed = 12345u)
{
    std::vector<int> tiles(static_cast<size_t>(cols * rows), 0);
    ProceduralSpawns spawns;

    std::mt19937 rng(seed);
    auto roll = [&rng](int lo, int hi) {
        return lo + static_cast<int>(rng() % static_cast<unsigned>(hi - lo + 1));
    };

    const int groundTop = rows - 2;
    const int groundBot = rows - 1;

    // Ground layer with occasional pits (max 3 tiles wide for jumpable gaps)
    for (int tx = 0; tx < cols; ++tx)
    {
        tiles[static_cast<size_t>(groundTop * cols + tx)] = 4;
        tiles[static_cast<size_t>(groundBot * cols + tx)] = 2;
    }
    // 2–3 pits
    for (int p = 0; p < 3; ++p)
    {
        int gapW = roll(2, 3);
        int start = roll(8 + p * 12, cols - 8 - gapW);
        for (int g = 0; g < gapW && start + g < cols; ++g)
        {
            tiles[static_cast<size_t>(groundTop * cols + start + g)] = 0;
            tiles[static_cast<size_t>(groundBot * cols + start + g)] = 0;
        }
    }

    // Floating platforms — chunk-based, Mario-style
    const int platformRows[] = { 3, 5, 7, 9 };
    int col = 4;
    while (col < cols - 6)
    {
        int platW = roll(2, 5);
        int rowIdx = roll(0, 3);
        int row = platformRows[rowIdx];
        if (col + platW > cols) break;

        for (int w = 0; w < platW; ++w)
        {
            tiles[static_cast<size_t>(row * cols + col + w)] = (row == platformRows[0]) ? 4 : 4;
            tiles[static_cast<size_t>((row + 1) * cols + col + w)] = 1;
        }

        spawns.patrollerPlatforms.push_back({ col, row, platW });
        if (row <= 5)
            spawns.flyerPlatforms.push_back({ col, row, platW });
        if (row >= 7)
            spawns.shooterPlatforms.push_back({ col, row, platW });
        spawns.jumperPlatforms.push_back({ col, row, platW });

        // Coins on some platforms
        if (roll(0, 2) == 0 && platW >= 2)
            spawns.coinTiles.push_back({ col + platW / 2, row - 1 });

        col += platW + roll(2, 5);
    }

    // Extra low platforms for variety
    for (int i = 0; i < 4; ++i)
    {
        int tx = roll(6 + i * 10, cols - 8);
        int platW = roll(2, 4);
        int row = 11;
        if (tx + platW >= cols) continue;
        bool overlaps = false;
        for (int w = 0; w < platW && !overlaps; ++w)
            if (tiles[static_cast<size_t>(row * cols + tx + w)] != 0) overlaps = true;
        if (overlaps) continue;
        for (int w = 0; w < platW; ++w)
        {
            tiles[static_cast<size_t>(row * cols + tx + w)] = 4;
            tiles[static_cast<size_t>((row + 1) * cols + tx + w)] = 1;
        }
        spawns.jumperPlatforms.push_back({ tx, row, platW });
        if (roll(0, 1) == 0)
            spawns.coinTiles.push_back({ tx + platW / 2, row - 1 });
    }

    // Coins above ground
    for (int i = 0; i < 6; ++i)
    {
        int tx = roll(2, cols - 3);
        if (tiles[static_cast<size_t>(groundTop * cols + tx)] != 0)
            spawns.coinTiles.push_back({ tx, groundTop - 1 });
    }

    return { tiles, spawns };
}

void GameplayState::buildLevel()
{
    // Procedural Mario-style level with seeded RNG
    unsigned seed = static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count());
    auto [tiles, spawns] = makeLevel(k_cols, k_rows, seed);
    m_tilemap.load(k_cols, k_rows, k_tileSize, tiles);

    auto tex = m_ctx->resources->loadTexture("tiles", "res/tiles.png", true);
    if (tex.isValid())
        m_tilemap.setTilesetTexture(tex.id, 8, 4);
    else
        std::cout << "[GameplayState] tiles.png not found — coloured quad fallback.\n";

    // Enemy textures
    auto tPatrol  = m_ctx->resources->loadTexture("enemy_patroller", "res/enemy_patroller.png", true);
    auto tFlyer   = m_ctx->resources->loadTexture("enemy_flyer",     "res/enemy_flyer.png",     true);
    auto tShooter = m_ctx->resources->loadTexture("enemy_shooter",   "res/enemy_shooter.png",   true);
    auto tJumper  = m_ctx->resources->loadTexture("enemy_jumper",    "res/enemy_jumper.png",    true);
    auto tProj    = m_ctx->resources->loadTexture("projectile",      "res/projectile.png",      true);

    auto toWorld = [](int col, int row, int width) {
        return glm::vec2{ (col + width * 0.5f) * static_cast<float>(k_tileSize),
                          row * static_cast<float>(k_tileSize) };
    };

    if (!spawns.patrollerPlatforms.empty())
    {
        auto& p = spawns.patrollerPlatforms.front();
        glm::vec2 pos = toWorld(p.col, p.row, p.width);
        float left  = p.col * static_cast<float>(k_tileSize);
        float right = (p.col + p.width) * static_cast<float>(k_tileSize);
        EnemySystem::spawnPatroller(m_world, pos, left, right, tPatrol.id);
    }
    if (!spawns.flyerPlatforms.empty())
    {
        auto& p = spawns.flyerPlatforms.front();
        glm::vec2 pos = toWorld(p.col, p.row, p.width);
        EnemySystem::spawnFlyer(m_world, pos, p.row * static_cast<float>(k_tileSize), tFlyer.id);
    }
    if (!spawns.shooterPlatforms.empty())
    {
        auto& p = spawns.shooterPlatforms.front();
        glm::vec2 pos = toWorld(p.col, p.row, p.width);
        EnemySystem::spawnShooter(m_world, pos, tShooter.id, tProj.id);
    }
    if (!spawns.jumperPlatforms.empty())
    {
        auto& p = spawns.jumperPlatforms.back();
        glm::vec2 pos = toWorld(p.col, p.row, p.width);
        EnemySystem::spawnJumper(m_world, pos, tJumper.id);
    }

    m_enemySystem.onProjectileHit = [this](Entity /*proj*/, Entity /*player*/) {
        damagePlayer(1);
    };
    m_enemySystem.onEnemyContact = [this](Entity /*enemy*/, Entity /*player*/) {
        damagePlayer(1);
    };
    m_enemySystem.onEnemyDeath = [this](World& world, Entity /*enemy*/, glm::vec2 pos) {
        spawnPowerUp(world, pos);
    };

    // Spawn coins at procedural positions (skip if coin texture missing)
    auto coinTexObj = m_ctx->resources->loadTexture("coin", "res/coin.png", true);
    if (coinTexObj.isValid())
    {
        GLuint coinTex = coinTexObj.id;
        auto spawnCoin = [&](int tx, int ty) {
            Entity c = m_world.create();
            m_world.add<TagComponent>(c, { "coin" });
            m_world.add<TransformComponent>(c, { .position = m_tilemap.tileToWorld(tx, ty), .scale = {1,1} });
            m_world.add<SpriteComponent>(c, { .textureID = coinTex,
                                              .size = {32,32},
                                              .tint = {1,1,1,1},
                                              .uvMin={0,0}, .uvMax={1,1}, .layer=2 });
            m_world.add<BoxColliderComponent>(c, { .offset={0,0}, .size={32,32}, .isTrigger=true });
        };
        for (const auto& ct : spawns.coinTiles)
            spawnCoin(ct.first, ct.second);
    }

    // hook physics overlap for coins and power-ups
    m_physics.onOverlap = [this](Entity a, Entity b) {
        auto isPlayer = [&](Entity e) {
            return m_world.has<TagComponent>(e) &&
                   m_world.get<TagComponent>(e).tag == "player";
        };
        auto tryCollect = [&](Entity player, Entity other) {
            if (!isPlayer(player)) return;
            if (m_world.has<TagComponent>(other) &&
                m_world.get<TagComponent>(other).tag == "coin")
            {
                m_world.destroy(other);
                m_score += 1;
            }
            else if (m_world.has<PowerUpComponent>(other))
            {
                applyPowerUp(m_world.get<PowerUpComponent>(other));
                m_world.destroy(other);
            }
        };
        tryCollect(a, b);
        tryCollect(b, a);
    };
}

void GameplayState::spawnPlayer()
{
    float startX = 3.f  * k_tileSize;
    float startY = 12.f * k_tileSize;

    m_player = m_world.create();

    auto playerTex = m_ctx->resources->loadTexture("knight", "res/knight.png", true);
    GLuint texID   = playerTex.isValid() ? playerTex.id : 0;

    m_world.add<TagComponent>(m_player, { "player" });
    m_world.add<TransformComponent>(m_player, {
        .position = { startX, startY },
        .scale    = { 1.f, 1.f }
    });
    m_world.add<SpriteComponent>(m_player, {
        .textureID = texID,
        .size      = { 48.f, 48.f },
        .tint      = { 1.f, 1.f, 1.f, 1.f },
        .uvMin     = { 0.f, 0.f },
        .uvMax     = { 0.25f, 0.25f },   // 4 rows now
        .layer     = 1
    });
    m_world.add<RigidbodyComponent>(m_player, {
        .velocity     = { 0.f, 0.f },
        .gravityScale = 1.f,
        .useGravity   = true
    });
    m_world.add<BoxColliderComponent>(m_player, {
        .offset = { 4.f, 0.f },
        .size   = { 28.f, 48.f }
    });
    m_world.add<PlayerComponent>(m_player);

    // Clips are registered by AttackSystem::registerClips in onEnter
    // after AttackComponent is added — just add an empty AnimatorComponent here
    AnimatorComponent anim;
    anim.currentClip = "idle";
    m_world.add<AnimatorComponent>(m_player, std::move(anim));
}

// -------------------------------------------------------------------------- //
//  Private — per-frame logic                                                 //
// -------------------------------------------------------------------------- //

void GameplayState::processPlayerInput(float dt)
{
    if (m_player == NULL_ENTITY) return;

    auto& rb = m_world.get<RigidbodyComponent>(m_player);
    auto& pc = m_world.get<PlayerComponent>(m_player);

    // Coyote time
    if (rb.onGround)
        m_coyoteTimer = k_coyoteTime;
    else if (m_coyoteTimer > 0.f)
        m_coyoteTimer -= dt;

    // Jump buffer
    if (m_ctx->input->isActionPressed("jump"))
        m_jumpBufferTimer = k_jumpBufferTime;
    else if (m_jumpBufferTimer > 0.f)
        m_jumpBufferTimer -= dt;

    // Horizontal (speed boost from power-up)
    float speed = pc.speed * (m_speedBoostTimer > 0.f ? 1.5f : 1.f);
    float targetVX = 0.f;
    if (m_ctx->input->isActionDown("left"))  { targetVX = -speed; pc.facingRight = false; }
    if (m_ctx->input->isActionDown("right")) { targetVX =  speed; pc.facingRight = true;  }
    rb.velocity.x = targetVX;

    // Jump
    if (m_coyoteTimer > 0.f && m_jumpBufferTimer > 0.f)
    {
        rb.velocity.y     = -pc.jumpForce;
        m_coyoteTimer     = 0.f;
        m_jumpBufferTimer = 0.f;
        m_ctx->resources->playSound("jump");
    }

    // Land sound
    if (rb.onGround && !m_wasOnGround)
        m_ctx->resources->playSound("land");
    m_wasOnGround = rb.onGround;

    // Sprite flip
    auto& tf   = m_world.get<TransformComponent>(m_player);
    tf.scale.x = pc.facingRight ? 1.f : -1.f;
}

void GameplayState::updateAnimatorState()
{
    if (m_player == NULL_ENTITY) return;
    if (!m_world.has<AnimatorComponent>(m_player)) return;

    // Don't override the attack animation while it's playing
    if (m_world.has<AttackComponent>(m_player) &&
        m_world.get<AttackComponent>(m_player).attacking)
        return;

    auto& rb   = m_world.get<RigidbodyComponent>(m_player);
    auto& anim = m_world.get<AnimatorComponent>(m_player);

    if (!rb.onGround)
        AnimationSystem::play(anim, "jump");
    else if (std::abs(rb.velocity.x) > 10.f)
        AnimationSystem::play(anim, "run");
    else
        AnimationSystem::play(anim, "idle");
}

void GameplayState::switchCharacter(CharacterType type)
{
    if (!m_world.has<AttackComponent>(m_player)) return;
    auto& ac = m_world.get<AttackComponent>(m_player);
    if (ac.character == type) return;

    ac.character      = type;
    ac.attacking      = false;
    ac.attackTimer    = 0.f;
    ac.attackCooldown = 0.f;

    std::string texName, texPath;
    switch (type)
    {
        case CharacterType::Knight: texName="knight"; texPath="res/knight.png"; break;
        case CharacterType::Ninja:  texName="ninja";  texPath="res/ninja.png";  break;
        case CharacterType::Robot:  texName="robot";  texPath="res/robot.png";  break;
    }

    auto tex = m_ctx->resources->loadTexture(texName, texPath, true);
    m_world.get<SpriteComponent>(m_player).textureID = tex.id;

    AttackSystem::registerClips(m_world, m_player, type);
    AnimationSystem::play(m_world.get<AnimatorComponent>(m_player), "idle");

    std::cout << "[GameplayState] Switched to " << texName << "\n";
}

void GameplayState::damagePlayer(int amount)
{
    if (m_player == NULL_ENTITY || !m_world.has<PlayerComponent>(m_player)) return;
    if (m_playerInvulnTimer > 0.f || m_invincibilityTimer > 0.f) return;  // invincible

    auto& pc = m_world.get<PlayerComponent>(m_player);
    pc.health = std::max(0, pc.health - amount);
    m_playerInvulnTimer = 1.2f;

    if (pc.health <= 0)
        m_fadingOut = true;  // return to menu on death
}

void GameplayState::spawnPowerUp(World& world, glm::vec2 position)
{
    // 25% chance to spawn a power-up on enemy death
    static std::mt19937 rng(static_cast<unsigned>(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    if (rng() % 4 != 0) return;

    PowerUpType type = PowerUpType::Health;
    switch (rng() % 3)
    {
        case 0: type = PowerUpType::Health;        break;
        case 1: type = PowerUpType::Speed;         break;
        case 2: type = PowerUpType::Invincibility; break;
    }

    Entity pu = world.create();
    world.add<TagComponent>(pu, { "powerup" });
    world.add<TransformComponent>(pu, { .position = position, .scale = {1.f, 1.f} });
    world.add<SpriteComponent>(pu, {
        .textureID = 0,
        .size      = { 28.f, 28.f },
        .tint      = type == PowerUpType::Health ? glm::vec4(1.f, 0.2f, 0.2f, 1.f)
                  : type == PowerUpType::Speed ? glm::vec4(1.f, 0.9f, 0.2f, 1.f)
                  : glm::vec4(0.5f, 0.5f, 1.f, 1.f),
        .layer     = 2
    });
    world.add<BoxColliderComponent>(pu, { .offset = {2.f, 2.f}, .size = {24.f, 24.f}, .isTrigger = true });
    world.add<PowerUpComponent>(pu, {
        .type     = type,
        .duration = type == PowerUpType::Health ? 0.f : 5.f
    });
}

void GameplayState::applyPowerUp(const PowerUpComponent& pu)
{
    if (m_player == NULL_ENTITY || !m_world.has<PlayerComponent>(m_player)) return;

    auto& pc = m_world.get<PlayerComponent>(m_player);

    switch (pu.type)
    {
        case PowerUpType::Health:
            pc.health = std::min(pc.maxHealth, pc.health + 1);
            break;
        case PowerUpType::Speed:
            m_speedBoostTimer = pu.duration;
            break;
        case PowerUpType::Invincibility:
            m_invincibilityTimer = pu.duration;
            break;
    }
}

void GameplayState::renderEntities()
{
    Renderer2D& r = *m_ctx->renderer;

    auto entities = m_world.view<TransformComponent, SpriteComponent>();
    std::stable_sort(entities.begin(), entities.end(),
        [this](Entity a, Entity b) {
            return m_world.get<SpriteComponent>(a).layer
                 < m_world.get<SpriteComponent>(b).layer;
        });

    for (Entity e : entities)
    {
        // Don't render entities that are destroyed (no DeathComponent = normal)
        if (m_world.has<DeathComponent>(e))
        {
            auto& dc = m_world.get<DeathComponent>(e);
            if (dc.timer <= 0.f) continue;  // about to be destroyed
        }

        auto& tf = m_world.get<TransformComponent>(e);
        auto& sp = m_world.get<SpriteComponent>(e);

        glm::vec4 tint = sp.tint;
        float scaleX = tf.scale.x;
        float scaleY = tf.scale.y;

        // Player hurt flash — red tint when invincible (just hit)
        if (e == m_player && m_playerInvulnTimer > 0.f)
        {
            float flash = 0.5f + 0.5f * std::sin(m_playerInvulnTimer * 25.f);
            tint = glm::vec4(1.f, 1.f - flash, 1.f - flash, sp.tint.a);
        }

        // Hit flash — red tint when enemy was just hit
        if (m_world.has<EnemyComponent>(e))
        {
            auto& ec = m_world.get<EnemyComponent>(e);
            if (ec.hitFlashTimer > 0.f)
                tint = glm::vec4(1.f, 0.3f, 0.3f, sp.tint.a);
        }

        // Death fade — shrink and fade out
        if (m_world.has<DeathComponent>(e))
        {
            auto& dc = m_world.get<DeathComponent>(e);
            float t = dc.timer / 0.3f;
            tint.a *= t;
            scaleX *= t;
            scaleY *= t;
        }

        glm::vec2 scaledSize = {
            sp.size.x * std::abs(scaleX),
            sp.size.y * std::abs(scaleY)
        };

        glm::vec2 drawPos = tf.position;
        if (scaleX < 0.f)
            drawPos.x += sp.size.x;

        if (sp.textureID != 0)
            r.drawQuad(drawPos, scaledSize, sp.textureID,
                       tint, tf.rotation, sp.uvMin, sp.uvMax);
        else
            r.drawQuad(drawPos, scaledSize, tint, tf.rotation);
    }
}

void GameplayState::renderFadeOverlay()
{
    if (m_fadeAlpha <= 0.f) return;

    Renderer2D& r   = *m_ctx->renderer;
    Camera&     cam = *m_ctx->camera;

    r.begin(cam);
        r.drawQuad({ 0.f, 0.f }, { 1280.f, 720.f },
                   glm::vec4(0.f, 0.f, 0.f, m_fadeAlpha));
    r.end();
}
