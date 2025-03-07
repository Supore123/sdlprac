#pragma once

#include <SDL2/SDL.h>
#include <string>

/**
 * IGameState
 *
 * Abstract interface that every concrete game state must implement.
 * States are owned and driven by GameStateMachine.
 *
 * Lifecycle per state:
 *   onEnter()  — called once when the state becomes active
 *   handleEvent() — called per SDL event each frame
 *   update()   — called once per frame with delta time (seconds)
 *   render()   — called once per frame after update
 *   onExit()   — called once when the state is popped or replaced
 */
class IGameState
{
public:
    virtual ~IGameState() = default;

    virtual void onEnter()                   = 0;
    virtual void onExit()                    = 0;
    virtual void handleEvent(SDL_Event& e)   = 0;
    virtual void update(float dt)            = 0;
    virtual void render()                    = 0;

    /** Human-readable name used for debugging and logging. */
    virtual std::string getName() const = 0;
};
