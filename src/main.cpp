#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <iostream>


// ---------------------------------------------------------------------- //
//  Entry point                                                            //
// ---------------------------------------------------------------------- //

int main()
{
    // ------------------------------------------------------------------ //
    //  SDL2 + OpenGL window creation (existing bare-bones setup)         //
    // ------------------------------------------------------------------ //

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
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

    SDL_GL_SetSwapInterval(1); // vsync

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
    //  Cleanup                                                            //
    // ------------------------------------------------------------------ //

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}
```

---

### Architecture summary
```
main.cpp
├── SDL2 + GLEW init (outside markers — your existing setup)
└── [ID 3] Wires ShaderManager + GameStateMachine into the loop
       ├── gsm.processTransition()   ← deferred, safe mid-frame
       ├── gsm.handleEvent(e)
       ├── gsm.update(dt)
       └── gsm.render()

ShaderManager [ID 1]
├── loadProgram(name, vert, frag) → compiles, links, caches
├── useProgram(name or GLuint)
└── setUniform(program, name, T) → overloaded for all glm types

GameStateMachine [ID 2]
├── push / pop / change → deferred via PendingTransition
├── processTransition() → applied once at start of each frame
└── Dispatches handleEvent / update / render to top-of-stack only
#include "ShaderManager.h"
#include "GameStateMachine.h"

// ---------------------------------------------------------------------- //
//  Concrete stub states — replace these with real implementations later  //
// ---------------------------------------------------------------------- //

class SplashState : public IGameState
{
public:
    void onEnter()               override { std::cout << "[SplashState] Enter\n"; }
    void onExit()                override { std::cout << "[SplashState] Exit\n";  }
    void handleEvent(SDL_Event&) override {}
    void update(float /*dt*/)    override {}
    void render()                override
    {
        glClearColor(0.05f, 0.05f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    std::string getName() const override { return "SplashState"; }
};

class MainMenuState : public IGameState
{
public:
    void onEnter()               override { std::cout << "[MainMenuState] Enter\n"; }
    void onExit()                override { std::cout << "[MainMenuState] Exit\n";  }
    void handleEvent(SDL_Event&) override {}
    void update(float /*dt*/)    override {}
    void render()                override
    {
        glClearColor(0.10f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    std::string getName() const override { return "MainMenuState"; }
};
    // ------------------------------------------------------------------ //
    //  Engine systems                                                     //
    // ------------------------------------------------------------------ //

    ShaderManager   shaderManager;
    GameStateMachine gsm;

    // Boot into the splash screen.
    // To load a real shader, call:
    //   shaderManager.loadProgram("basic", "res/basic.vert", "res/basic.frag");
    gsm.push(std::make_unique<SplashState>());

    // ------------------------------------------------------------------ //
    //  Game loop                                                          //
    // ------------------------------------------------------------------ //

    bool     running    = true;
    Uint64   now        = SDL_GetPerformanceCounter();
    Uint64   last       = 0;
    double   freq       = static_cast<double>(SDL_GetPerformanceFrequency());

    // Demo: switch to MainMenuState after 3 seconds
    float    stateTimer = 0.0f;
    bool     switched   = false;

    while (running)
    {
        last = now;
        now  = SDL_GetPerformanceCounter();
        float dt = static_cast<float>((now - last) / freq);

        // -- Process any deferred state transition ---------------------- //
        gsm.processTransition();

        // -- Event handling -------------------------------------------- //
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                running = false;

            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
                running = false;

            gsm.handleEvent(e);
        }

        // -- Update ---------------------------------------------------- //
        gsm.update(dt);

        // Demo transition: splash → main menu after 3 s
        if (!switched)
        {
            stateTimer += dt;
            if (stateTimer >= 3.0f)
            {
                gsm.change(std::make_unique<MainMenuState>());
                switched = true;
            }
        }

        // -- Render ---------------------------------------------------- //
        gsm.render();
        SDL_GL_SwapWindow(window);

        if (gsm.isEmpty())
            running = false;
    }
