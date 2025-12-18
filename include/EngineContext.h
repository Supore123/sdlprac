#pragma once

// (Unchanged from previous commit — re-issued for clean project state)

class ShaderManager;
class ResourceManager;
class InputManager;
class GameStateMachine;
class Camera;
class Renderer2D;

/**
 * EngineContext
 *
 * Non-owning pointers to every engine subsystem. Passed to every IGameState.
 * All six pointers MUST be assigned before gsm.push() is called.
 *
 * Per-state data (World, Tilemap, PhysicsWorld) lives inside each GameState
 * as member variables — not here. This keeps state lifetimes clean and avoids
 * shared mutable state between states.
 */
struct EngineContext
{
    ShaderManager*    shaders   = nullptr;
    ResourceManager*  resources = nullptr;
    InputManager*     input     = nullptr;
    GameStateMachine* gsm       = nullptr;
    Camera*           camera    = nullptr;
    Renderer2D*       renderer  = nullptr;
};
