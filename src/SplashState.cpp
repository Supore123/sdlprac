
#include "SplashState.h"
#include "GameplayState.h"   // transition target
#include "EngineContext.h"
#include "Camera.h"
#include "Renderer2D.h"
#include "InputManager.h"
#include "GameStateMachine.h"

// -------------------------------------------------------------------------- //
//  Lifecycle                                                                  //
// -------------------------------------------------------------------------- //

void SplashState::onEnter()
{
    m_elapsed    = 0.f;
    m_fadeAlpha  = 1.f;    // start black, fade in
    m_fadingIn   = true;
    m_fadingOut  = false;
    std::cout << "[SplashState] Enter\n";
}

void SplashState::onExit()
{
    std::cout << "[SplashState] Exit\n";
}

// -------------------------------------------------------------------------- //
//  Event                                                                      //
// -------------------------------------------------------------------------- //

void SplashState::handleEvent(SDL_Event& /*e*/)
{
    // InputManager is driven by the main loop — states only read it.
}

// -------------------------------------------------------------------------- //
//  Update                                                                     //
// -------------------------------------------------------------------------- //

void SplashState::update(float dt)
{
    // ── Fade in ──────────────────────────────────────────────────────────────
    if (m_fadingIn)
    {
        m_fadeAlpha -= m_fadeSpeed * dt;
        if (m_fadeAlpha <= 0.f)
        {
            m_fadeAlpha = 0.f;
            m_fadingIn  = false;
        }
        return;   // don't accept input while fading in
    }

    m_elapsed += dt;

    // ── Trigger fade-out → GameplayState ─────────────────────────────────────
    bool skip = m_ctx->input->isActionPressed("confirm")
             || m_ctx->input->isActionPressed("up")
             || m_ctx->input->isActionPressed("right");

    // Also auto-advance after 3 seconds
    if ((skip || m_elapsed >= 3.f) && !m_fadingOut)
        m_fadingOut = true;

    // ── Fade out ─────────────────────────────────────────────────────────────
    if (m_fadingOut)
    {
        m_fadeAlpha += m_fadeSpeed * dt;
        if (m_fadeAlpha >= 1.f)
        {
            m_fadeAlpha = 1.f;
            // Scene transition: replace SplashState with GameplayState
            m_ctx->gsm->change(std::make_unique<GameplayState>(m_ctx));
        }
    }
}

// -------------------------------------------------------------------------- //
//  Render                                                                     //
// -------------------------------------------------------------------------- //

void SplashState::render()
{
    glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Renderer2D& r   = *m_ctx->renderer;
    Camera&     cam = *m_ctx->camera;

    r.begin(cam);

        // Title card quads — orange bar + white centred box
        r.drawQuad({ 0.f, 310.f }, { 1280.f, 8.f },
                   glm::vec4(0.95f, 0.50f, 0.15f, 1.f));   // horizontal accent line
        r.drawQuad({ 0.f, 402.f }, { 1280.f, 8.f },
                   glm::vec4(0.95f, 0.50f, 0.15f, 1.f));

        // "PRESS SPACE" indicator boxes
        r.drawQuad({ 530.f, 420.f }, { 220.f, 50.f },
                   glm::vec4(0.18f, 0.18f, 0.22f, 0.9f));
        r.drawQuad({ 535.f, 425.f }, { 210.f, 40.f },
                   glm::vec4(0.95f, 0.50f, 0.15f, 0.85f));

        // Demo quads (show the renderer is working)
        r.drawQuad({  50.f, 550.f }, { 100.f, 100.f },
                   glm::vec4(0.95f, 0.20f, 0.20f, 1.f));   // red
        r.drawQuad({ 565.f, 260.f }, { 150.f, 150.f },
                   glm::vec4(1.00f, 1.00f, 1.00f, 0.9f));  // white
        r.drawQuad({1130.f, 550.f }, { 100.f, 100.f },
                   glm::vec4(0.20f, 0.50f, 0.95f, 1.f), 45.f);  // blue, rotated

    r.end();

    // ── Fade overlay ─────────────────────────────────────────────────────────
    if (m_fadeAlpha > 0.f)
    {
        r.begin(cam);
            r.drawQuad({ 0.f, 0.f }, { 1280.f, 720.f },
                       glm::vec4(0.f, 0.f, 0.f, m_fadeAlpha));
        r.end();
    }
}
