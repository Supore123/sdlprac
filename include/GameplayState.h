#pragma once
#include "IGameState.h"
#include "ECS.h"
#include "Tilemap.h"
#include "PhysicsWorld.h"
#include "AnimationSystem.h"
#include "HUD.h"
#include <string>
struct EngineContext;
/**
 * GameplayState
 *
 * Self-contained platformer level. Owns its own ECS World, Tilemap,
 * PhysicsWorld, AnimationSystem, and HUD — no global state required.
 *
 * ── What runs ────────────────────────────────────────────────────────────────
 *   ECS:       Player entity + future NPC / pickup entities
 *   Tilemap:   25 × 15 grid of 48px tiles (hardcoded for the demo, swap for
 *              a JSON loader when you have asset files)
 *   Physics:   Gravity, horizontal movement, jump, tile collision
 *   Animation: Idle / run / jump clips (UV-based, no texture file needed
 *              for coloured-quad rendering)
 *   HUD:       Health pip bar (top-left), scored pause overlay (ESC)
 *   Transition:ESC → pop back to SplashState
 *
 * ── Controls ─────────────────────────────────────────────────────────────────
 *   A / D or ← / →   Walk left / right
 *   Space             Jump (only when on ground)
 *   ESC               Return to previous state (SplashState)
 */
class GameplayState : public IGameState
{
public:
    explicit GameplayState(EngineContext* ctx);
    void onEnter()              override;
    void onExit()               override;
    void handleEvent(SDL_Event& e) override;
    void update(float dt)       override;
    void render()               override;
    std::string getName() const override { return "GameplayState"; }
private:
    EngineContext*  m_ctx = nullptr;
    // ── Per-state systems ────────────────────────────────────────────────────
    World           m_world;
    Tilemap         m_tilemap;
    PhysicsWorld    m_physics;
    AnimationSystem m_animSystem;
    HUD             m_hud;
    // ── Scene-transition fade ────────────────────────────────────────────────
    float m_fadeAlpha     = 1.f;   // 1=black, 0=visible
    bool  m_fadingIn      = true;  // true during enter fade-in
    bool  m_fadingOut     = false; // true during exit fade-out
    float m_fadeSpeed     = 2.5f;  // alpha units per second
    // ── Tracked entities ─────────────────────────────────────────────────────
    Entity m_player = NULL_ENTITY;
    // ── Helpers ──────────────────────────────────────────────────────────────
    void buildLevel();
    void spawnPlayer();
    void processPlayerInput(float dt);
    void updateAnimatorState();
    void renderEntities();
    void renderFadeOverlay();
};
    float m_coyoteTimer     = 0.f;   // counts down after leaving ground
    float m_jumpBufferTimer = 0.f;   // counts down after jump pressed in air
