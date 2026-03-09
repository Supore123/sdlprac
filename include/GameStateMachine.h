



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
    // ------------------------------------------------------------------ //
    //  Transition requests (deferred until processTransition())           //
    // ------------------------------------------------------------------ //

    /** Push a new state on top of the stack. */
    void push(std::unique_ptr<IGameState> state);

    /** Pop the top state from the stack. */
    void pop();

    /**
     * Replace the current top state with a new one.
     * Equivalent to pop() + push() in a single deferred step.
     */
    void change(std::unique_ptr<IGameState> state);

    // ------------------------------------------------------------------ //
    //  Per-frame driver — call these in order from your main loop         //
    // ------------------------------------------------------------------ //

    /** Apply any pending transition before the frame begins. */
    void processTransition();

    void handleEvent(SDL_Event& e);
    void update(float dt);
    void render();

    // ------------------------------------------------------------------ //
    //  Queries                                                             //
    // ------------------------------------------------------------------ //

    bool        isEmpty()       const { return m_stack.empty(); }
    std::string currentName()   const;
private:
    enum class TransitionType { None, Push, Pop, Change };

    struct PendingTransition
    {
        TransitionType              type  = TransitionType::None;
        std::unique_ptr<IGameState> state = nullptr;
    };

    std::stack<std::unique_ptr<IGameState>> m_stack;
    PendingTransition                       m_pending;
};
