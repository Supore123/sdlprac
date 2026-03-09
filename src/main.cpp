



// ─────────────────────────────────────────────────────────────────────────── //
//  Existing includes — SDL2 + GLEW + standard library                        //
// ─────────────────────────────────────────────────────────────────────────── //
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <iostream>

// These were already present and correct in the original:
#include "ShaderManager.h"
#include "GameStateMachine.h"
#include "ResourceManager.h"
#include "InputManager.h"

// Moving these to the TOP of main.cpp so all types are visible 
// before they are used, fixing the "does not name a type" errors.
#include "EngineContext.h"   
#include "SplashState.h"     
#include "Camera.h"
#include "Renderer2D.h"

int main()
{
    // ─────────────────────────────────────────────────────────────────────── //
    //  SDL2 init — original, unchanged                                        //
    // ─────────────────────────────────────────────────────────────────────── //

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

    SDL_GL_SetSwapInterval(1);   // VSync

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

    // ─────────────────────────────────────────────────────────────────────── //
    //  Engine systems (ShaderManager–InputManager were already here)          //
    // ─────────────────────────────────────────────────────────────────────── //

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

    // ─────────────────────────────────────────────────────────────────────── //
    //  Camera & Renderer Setup                                                //
    // ─────────────────────────────────────────────────────────────────────── //
    
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

    // ─────────────────────────────────────────────────────────────────────── //
    //  EngineContext Setup & Push Initial State                               //
    // ─────────────────────────────────────────────────────────────────────── //

    EngineContext ctx;
    ctx.shaders   = &shaderManager;
    ctx.resources = &resourceManager;
    ctx.input     = &inputManager;
    ctx.gsm       = &gsm;
    ctx.camera    = &camera;    
    ctx.renderer  = &renderer;  

    gsm.push(std::make_unique<SplashState>(&ctx));

    // ─────────────────────────────────────────────────────────────────────── //
    //  Main Game Loop                                                         //
    // ─────────────────────────────────────────────────────────────────────── //

    bool   running = true;
    Uint64 now     = SDL_GetPerformanceCounter();
    Uint64 last    = now;   // Seeded correctly to avoid dt spike
    double freq    = static_cast<double>(SDL_GetPerformanceFrequency());

    while (running)
    {
        last = now;
        now  = SDL_GetPerformanceCounter();
        float dt = static_cast<float>(static_cast<double>(now - last) / freq);

        // Clear single-frame flags BEFORE processing new events
        inputManager.update();
        gsm.processTransition();

        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            // Handle resizing directly in the event loop
            if (e.type == SDL_WINDOWEVENT &&
                e.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                float w = static_cast<float>(e.window.data1);
                float h = static_cast<float>(e.window.data2);
                camera.setOrthoSize(w, h);
                camera.setAspect(w / h);
                glViewport(0, 0, e.window.data1, e.window.data2);
            }

            // Process input events
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

    // ─────────────────────────────────────────────────────────────────────── //
    //  Cleanup and Exit                                                       //
    // ─────────────────────────────────────────────────────────────────────── //

    // Shut down engine subsystems while GL Context is still alive
    renderer.shutdown();
    resourceManager.releaseAll();
    shaderManager.deleteAll();

    // Destroy SDL and GL Context
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return EXIT_SUCCESS;
} // End of main()

