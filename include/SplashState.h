#pragma once


#include <GL/glew.h>
#include <glm/glm.hpp>
#include <SDL2/SDL.h>
#include <string>
#include <iostream>

#include "IGameState.h"
#include "EngineContext.h"

class SplashState : public IGameState
{
public:
    explicit SplashState(EngineContext* ctx) : m_ctx(ctx) {}

    void onEnter()              override;
    void onExit()               override;
    void handleEvent(SDL_Event& e) override;
    void update(float dt)       override;
    void render()               override;

    std::string getName() const override { return "SplashState"; }

private:
    EngineContext* m_ctx      = nullptr;
    float          m_elapsed  = 0.f;

    // Scene transition fade
    float m_fadeAlpha  = 1.f;
    bool  m_fadingIn   = true;
    bool  m_fadingOut  = false;
    float m_fadeSpeed  = 2.5f;   // alpha per second
};
