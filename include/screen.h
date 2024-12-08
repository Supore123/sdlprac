// Includes
#include "app.h"
#include <SDL2/SDL.h>
#include "version.h"

//Defines
#define MAIN_SCREEN_WIDTH       640
#define MAIN_SCREEN_HEIGHT      480
#define IMAGE_DIR               "imgs/"

// Variables
typedef struct Window
{
    SDL_Window *window; // Background Window
    SDL_Surface *screenSurface;
    SDL_Surface *imageSurface;
}Window_t;

//Func Defines
appStatus_t screenInit(void);
appStatus_t loadImage(void);