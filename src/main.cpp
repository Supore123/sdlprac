#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <iostream>

#include "ShaderManager.h"
#include "GameStateMachine.h"
#include "ResourceManager.h"
#include "InputManager.h"




int main()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) != 0)
    {
        std::cerr << "[main] SDL_Init failed: " << SDL_GetError() << "\n";
        return EXIT_FAILURE;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   24);

    SDL_Window* window = SDL_CreateWindow(
        "Game",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    if (!window)
    {
        std::cerr << "[main] SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext)
    {
        std::cerr << "[main] SDL_GL_CreateContext failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_GL_SetSwapInterval(1);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "[main] glewInit failed.\n";
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ------------------------------------------------------------------ //
    //  Engine systems                                                      //
    // ------------------------------------------------------------------ //

    ShaderManager    shaderManager;
    GameStateMachine gsm;
    ResourceManager  resourceManager;
    InputManager     inputManager;


    resourceManager.initAudio();

    inputManager.bindAction("confirm", SDL_SCANCODE_SPACE);
    inputManager.bindAction("back",    SDL_SCANCODE_ESCAPE);
    inputManager.bindAction("up",      SDL_SCANCODE_W);
    inputManager.bindAction("down",    SDL_SCANCODE_S);
    inputManager.bindAction("left",    SDL_SCANCODE_A);
    inputManager.bindAction("right",   SDL_SCANCODE_D);

    EngineContext ctx;
    ctx.shaders   = &shaderManager;
    ctx.resources = &resourceManager;
    ctx.input     = &inputManager;
    ctx.gsm       = &gsm;


    gsm.push(std::make_unique<SplashState>(&ctx));

    bool   running = true;
    Uint64 now     = SDL_GetPerformanceCounter();
    Uint64 last    = 0;
    double freq    = static_cast<double>(SDL_GetPerformanceFrequency());

    while (running)
    {
        last = now;
        now  = SDL_GetPerformanceCounter();
        float dt = static_cast<float>((now - last) / freq);

        inputManager.update();
        gsm.processTransition();

        SDL_Event e;
        while (SDL_PollEvent(&e))
        {

            inputManager.processEvent(e);
            gsm.handleEvent(e);
        }

        if (inputManager.quit() || inputManager.isActionPressed("back"))
            running = false;

        gsm.update(dt);
        gsm.render();
        SDL_GL_SwapWindow(window);

        if (gsm.isEmpty())
            running = false;
    }

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}
#include "Camera.h"
#include "Renderer2D.h"
struct EngineContext
{
    ShaderManager*    shaders   = nullptr;
    ResourceManager*  resources = nullptr;
    InputManager*     input     = nullptr;
    GameStateMachine* gsm       = nullptr;
    Camera*           camera    = nullptr;
    Renderer2D*       renderer  = nullptr;
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
            std::cout << "[SplashState] Confirm — wire your MenuState here.\n";
    }

    void render() override
    {
        glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Renderer2D demo — three coloured quads visible immediately on launch
        Renderer2D& r   = *m_ctx->renderer;
        Camera&     cam = *m_ctx->camera;

        r.begin(cam);
            // A white 200x200 centred box
            r.drawQuad({ 540.f, 260.f }, { 200.f, 200.f },
                       glm::vec4(1.00f, 1.00f, 1.00f, 1.f));
            // Red box bottom-left
            r.drawQuad({  50.f, 550.f }, { 120.f, 120.f },
                       glm::vec4(0.95f, 0.20f, 0.20f, 1.f));
            // Blue box bottom-right, rotated 45°
            r.drawQuad({1060.f, 550.f }, { 120.f, 120.f },
                       glm::vec4(0.20f, 0.50f, 0.95f, 1.f), 45.f);
        r.end();
    }

    std::string getName() const override { return "SplashState"; }

private:
    EngineContext* m_ctx = nullptr;
};
    Camera     camera;
    Renderer2D renderer;

    camera.setMode(Camera::Mode::Orthographic);
    camera.setOrthoSize(1280.f, 720.f);

    if (!renderer.init(shaderManager))
    {
        std::cerr << "[main] Renderer2D init failed.\n";
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_FAILURE;
    }
    ctx.camera   = &camera;
    ctx.renderer = &renderer;
            if (e.type == SDL_WINDOWEVENT &&
                e.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                float w = static_cast<float>(e.window.data1);
                float h = static_cast<float>(e.window.data2);
                camera.setOrthoSize(w, h);
                camera.setAspect(w / h);
                glViewport(0, 0, e.window.data1, e.window.data2);
            }
