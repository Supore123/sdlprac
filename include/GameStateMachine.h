



#pragma once

#include "IGameState.h"

#include <memory>
#include <stack>
#include <string>
/**
 * GameStateMachine
 *
 * Manages a stack of IGameState instances, executing only the top state
 * each frame. Supports three transition modes:
 *
 *   push()   — Pause the current state and push a new one on top.
 *              Useful for overlays, pause menus, etc.
 *
 *   pop()    — Remove the top state and resume the one beneath it.
 *
 *   change() — Replace the current top state entirely (pop + push).
 *              Useful for full-screen transitions (menu → gameplay).
 *
 * Transitions are deferred: the actual stack modification happens at the
 * start of the next processTransition() call (once per frame), preventing
 * mid-frame corruption while iterating through update/render.
 */
class GameStateMachine
{
public:
    GameStateMachine()  = default;
    ~GameStateMachine() = default;

    GameStateMachine(const GameStateMachine&)            = delete;
    GameStateMachine& operator=(const GameStateMachine&) = delete;
