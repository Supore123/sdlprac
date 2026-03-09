



#pragma once
#include "IGameState.h"
#include "ECS.h"
#include "Tilemap.h"
#include "PhysicsWorld.h"
#include "AnimationSystem.h"
#include "HUD.h"
#include "EnemySystem.h"
#include "AttackSystem.h"
#include <string>

struct EngineContext;
/**
 * GameplayState
 *
 * Self-contained platformer level. Owns its own ECS World, Tilemap,
 * PhysicsWorld, AnimationSystem, HUD, EnemySystem, and AttackSystem.
 *
 * Controls:
 *   A / D       Walk left / right
 *   Space       Jump (coyote time + jump buffer)
 *   J           Attack
 *   1 / 2 / 3   Switch character (Knight / Ninja / Robot)
 *   ESC         Return to previous state
 */
class GameplayState : public IGameState
{
public:
    explicit GameplayState(EngineContext* ctx);
    void onEnter()                 override;
    void onExit()                  override;
    void handleEvent(SDL_Event& e) override;
    void update(float dt)          override;
    void render()                  override;
    std::string getName() const    override { return "GameplayState"; }
private:
    EngineContext*  m_ctx = nullptr;

    // ── Per-state systems ────────────────────────────────────────────────────
    World           m_world;
    Tilemap         m_tilemap;
    PhysicsWorld    m_physics;
    AnimationSystem m_animSystem;
    HUD             m_hud;
    EnemySystem     m_enemySystem;
    AttackSystem    m_attackSystem;

    // ── Scene-transition fade ────────────────────────────────────────────────
    float m_fadeAlpha  = 1.f;
    bool  m_fadingIn   = true;
    bool  m_fadingOut  = false;
    float m_fadeSpeed  = 2.5f;

    // ── Tracked entities ─────────────────────────────────────────────────────
    Entity m_player = NULL_ENTITY;

    // ── Movement feel ────────────────────────────────────────────────────────
    float m_coyoteTimer     = 0.f;
    float m_jumpBufferTimer = 0.f;

    // ── Player state ─────────────────────────────────────────────────────────
    float m_playerInvulnTimer  = 0.f;   // invincibility after hit
    float m_speedBoostTimer    = 0.f;   // power-up duration
    float m_invincibilityTimer = 0.f;   // power-up duration

    // ── Camera ───────────────────────────────────────────────────────────────
    glm::vec2 m_camOffset = { 0.f, 0.f };

    // ── Audio ────────────────────────────────────────────────────────────────
    bool m_wasOnGround = false;

    // ── Gameplay data ────────────────────────────────────────────────────────
    int  m_score = 0;

    // ── Attack FX textures ───────────────────────────────────────────────────
    GLuint m_fxSwordTex    = 0;
    GLuint m_fxShurikenTex = 0;
    GLuint m_fxLaserTex    = 0;
    GLuint m_fxHitTex      = 0;
    GLuint m_fontTexID     = 0;
