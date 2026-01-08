//                  convenience header.
//
// The original app.h declared a void appInit(void) function that was never
// defined or called anywhere in the project. It had no connection to the
// SDL2/OpenGL engine.
//
// app.h is now a single convenience header that game states and other
// translation units can include to get the complete EngineContext definition
// along with the concrete system headers they most often need. This avoids
// each state having to manually repeat the same five includes.
#pragma once

#include "EngineContext.h"   // struct EngineContext
#include "ShaderManager.h"
#include "ResourceManager.h"
#include "InputManager.h"
#include "GameStateMachine.h"
#include "Camera.h"
#include "Renderer2D.h"
