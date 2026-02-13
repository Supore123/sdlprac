#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <iostream>

#include "ShaderManager.h"
#include "GameStateMachine.h"
#include "ResourceManager.h"
#include "InputManager.h"


// ---------------------------------------------------------------------- //
//  Concrete stub states                                                   //
// ---------------------------------------------------------------------- //


// ---------------------------------------------------------------------- /

/**
 * EngineContext
 * Thin aggregate passed into every state so systems are accessible
 * without global singletons. Extend as more systems are added.
 */
struct EngineContext
{
    ShaderManager*   shaders   = nullptr;
    ResourceManager* resources = nullptr;
    InputManager*    input     = nullptr;
    GameStateMachine* gsm      = nullptr;
};


class SplashState : public IGameState
{
public:
    explicit SplashState(EngineContext* ctx) : m_ctx(ctx) {}

    void onEnter() override { std::cout << "[SplashState] Enter\n"; }
    void onExit()  override { std::cout << "[SplashState] Exit\n";  }

    void handleEvent(SDL_Event& e) override
    {
        m_ctx->input->processEvent(e);
    }

    void update(float /*dt*/) override
    {
        if (m_ctx->input->isActionPressed("confirm"))
            std::cout << "[SplashState] Confirm pressed — wire in your MenuState here.\n";
    }

    void render() override
    {
        glClearColor(0.05f, 0.05f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    std::string getName() const override { return "SplashState"; }

private:
    EngineContext* m_ctx = nullptr;
};

