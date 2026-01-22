#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <iostream>

#include "ShaderManager.h"
#include "GameStateMachine.h"


// ---------------------------------------------------------------------- //
//  Concrete stub states (now receive engine context)                      //
// ---------------------------------------------------------------------- //


class SplashState : public IGameState
{
public:
    void onEnter()  override { std::cout << "[SplashState] Enter\n"; }
    void onExit()   override { std::cout << "[SplashState] Exit\n";  }
    void handleEvent(SDL_Event& e) override
    {
    }
    void update(float /*dt*/) override
    {
    }
    void render() override
    {
        glClearColor(0.05f, 0.05f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    std::string getName() const override { return "SplashState"; }
private:
};

// ---------------------------------------------------------------------- //
//  Entry point                                                            //
// ---------------------------------------------------------------------- //

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

    // ------------------------------------------------------------------ //
    //  Engine systems                                                     //
    // ------------------------------------------------------------------ //

    ShaderManager    shaderManager;
    GameStateMachine gsm;


    // ------------------------------------------------------------------ //
    //  Game loop                                                          //
    // ------------------------------------------------------------------ //

    bool   running = true;
    Uint64 now     = SDL_GetPerformanceCounter();
    Uint64 last    = 0;
    double freq    = static_cast<double>(SDL_GetPerformanceFrequency());

    while (running)
    {
        last = now;
        now  = SDL_GetPerformanceCounter();
        float dt = static_cast<float>((now - last) / freq);


        gsm.processTransition();

        SDL_Event e;
        while (SDL_PollEvent(&e))
        {

            gsm.handleEvent(e);
        }

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
#include "ResourceManager.h"
#include "InputManager.h"
/**
 * EngineContext
 *
 * Thin aggregate passed by pointer into every state so they can access
 * all engine systems without global singletons or tight coupling.
 * Extend this struct as more systems are added.
 */
struct EngineContext
{
    ShaderManager*   shaders   = nullptr;
    ResourceManager* resources = nullptr;
    InputManager*    input     = nullptr;
    GameStateMachine* gsm      = nullptr;
};
    explicit SplashState(EngineContext* ctx) : m_ctx(ctx) {}
        // Route raw SDL events into InputManager
        m_ctx->input->processEvent(e);
        // Skip splash instantly when Space / A-button pressed
        if (m_ctx->input->isActionPressed("confirm"))
            m_ctx->gsm->change(std::make_unique<SplashState>(m_ctx)); // swap in real MenuState
    EngineContext* m_ctx = nullptr;
    ResourceManager  resourceManager;
    InputManager     inputManager;

    resourceManager.initAudio();

    // Register named action bindings — edit these to match your control scheme
    inputManager.bindAction("confirm", SDL_SCANCODE_SPACE);
    inputManager.bindAction("back",    SDL_SCANCODE_ESCAPE);
    inputManager.bindAction("up",      SDL_SCANCODE_W);
    inputManager.bindAction("down",    SDL_SCANCODE_S);
    inputManager.bindAction("left",    SDL_SCANCODE_A);
    inputManager.bindAction("right",   SDL_SCANCODE_D);

    // Pack all systems into the context passed to every state
    EngineContext ctx;
    ctx.shaders   = &shaderManager;
    ctx.resources = &resourceManager;
    ctx.input     = &inputManager;
    ctx.gsm       = &gsm;

    gsm.push(std::make_unique<SplashState>(&ctx));
        // Clear single-frame input flags before polling new events
        inputManager.update();
            // InputManager intercepts quit + input events
            inputManager.processEvent(e);
            if (inputManager.quit() ||
                inputManager.isActionPressed("back"))
                running = false;
