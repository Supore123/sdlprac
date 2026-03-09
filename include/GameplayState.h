



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
