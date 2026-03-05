#pragma once

#include <SDL2/SDL.h>
#include <string>

class IGameState
{
public:
    virtual ~IGameState() = default;

    virtual void onEnter()                 = 0;
    virtual void onExit()                  = 0;
    virtual void handleEvent(SDL_Event& e) = 0;
    virtual void update(float dt)          = 0;
    virtual void render()                  = 0;

    virtual std::string getName() const = 0;
};
