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
    //  Cleanup — original pattern, explicit renderer shutdown added           //
    // ─────────────────────────────────────────────────────────────────────── //


    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}
//                  SplashState to the TOP of main.cpp so all types are
//                  visible before they are used.
//
// BUG: In the original, these four #includes and the definitions of
// EngineContext / SplashState appeared AFTER return EXIT_SUCCESS.
// The compiler never reached them, so EngineContext and SplashState were
// undefined types at their point of use — the project could not compile.
#include "EngineContext.h"   // struct EngineContext (own header, COMMIT 01)
#include "SplashState.h"     // class  SplashState  (own header, COMMIT 02)
#include "Camera.h"
#include "Renderer2D.h"
    //                   EngineContext and pushing the first state.
    //
    // BUG: In the original, Camera and Renderer2D were declared AFTER
    // return EXIT_SUCCESS — dead code. ctx.camera and ctx.renderer were
    // therefore always nullptr. SplashState::render() dereferenced both
    // immediately (* m_ctx->renderer, * m_ctx->camera), which would have
    // been a guaranteed null-pointer crash on the very first frame.
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
    //                   then push the first state.
    //
    // BUG: In the original, ctx.camera and ctx.renderer were assigned after
    // return EXIT_SUCCESS and therefore never set. Now Camera and Renderer2D
    // are live (see COMMIT 05) and assigned here before gsm.push().
    EngineContext ctx;
    ctx.shaders   = &shaderManager;
    ctx.resources = &resourceManager;
    ctx.input     = &inputManager;
    ctx.gsm       = &gsm;
    ctx.camera    = &camera;    // was always nullptr in the original
    ctx.renderer  = &renderer;  // was always nullptr in the original

    gsm.push(std::make_unique<SplashState>(&ctx));
