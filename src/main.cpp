



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
