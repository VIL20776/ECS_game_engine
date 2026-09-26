#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "game.hpp"


Game game;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) 
{
    game.setup("./components.toml", "./systems.lua");

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) 
{
    if (event->type == SDL_EVENT_QUIT)
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */

    double dt = ((double)SDL_GetTicks()) / 1000.0;
    game.input(event, dt);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) 
{
    double dt = ((double)SDL_GetTicks()) / 1000.0;
    if (game.isRunning())
        game.update(dt);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) 
{
    game.quit();
}
