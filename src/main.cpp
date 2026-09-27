#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "game.hpp"

namespace {

constexpr std::string_view kDefaultComponentFile = "./components.toml";
constexpr std::string_view kDefaultSystemsFile = "./systems.lua";

} // namespace

game::Game ctx;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
    (void)appstate;
    (void)argc;
    (void)argv;
    if (const auto result = ctx.setup(kDefaultComponentFile, kDefaultSystemsFile); !result.has_value()) {
        return SDL_APP_FAILURE;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    (void)appstate;
    if (event == nullptr) {
        return SDL_APP_CONTINUE;
    }
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    ctx.input(event, 0.0);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    (void)appstate;
    if (ctx.isRunning()) {
        ctx.update(0.0);
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    (void)appstate;
    (void)result;
    ctx.quit();
}
